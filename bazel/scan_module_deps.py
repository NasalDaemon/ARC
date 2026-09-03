#!/usr/bin/env python3
"""Scan C++20 module source files to build a precise dependency map.

Walks one or more workspace roots scanning:
  - .ixx files  (C++ module interface units)
  - .ixx.arc    (ARC DSL — valid C++20 module preamble, 'import arc;' injected)
  - .cpp / .cc  (consumers, implementation units, and arc-embed module sources)

Each root is supplied as:
  --root=<absolute_path>|<short_path_prefix>|<repo_label_prefix>

  short_path_prefix is prepended to MODULE_SOURCES / FILE_DIRECT_DEPS keys.
  Empty for the main workspace. For files under an external repo X with
  canonical name X+, use prefix "../X+/" so keys match File.short_path.

  repo_label_prefix is prepended to MODULE_PROVIDER_TARGETS labels emitted
  for modules found under this root. Empty for the main workspace; "@arc"
  for the @arc external repo.

For arc-embed .cpp files the arc block (between arc-begin / arc-end markers)
is extracted to a temporary .ixx file so that clang-scan-deps can scan it as
a module interface unit. 'import arc;' is then injected for arc-type files.

All files are scanned in a single clang-scan-deps --format=p1689 invocation.
The Python regex scanner is kept only as a fallback for files where
clang-scan-deps returns an error.

Output: a Starlark .bzl file printed to stdout containing:
  MODULE_DIRECT_DEPS      — {module_name: [dep_module_names, ...]}
  FILE_DIRECT_DEPS        — {short_path: [dep_module_names, ...]}
  MODULE_SOURCES          — {short_path: module_name}
  MODULE_PROVIDER_TARGETS — {module_name: bazel_target_label}
"""

import json
import os
import subprocess
import sys
import tempfile

_SKIP_DIRS = frozenset({
    "bazel-bin", "bazel-out", "bazel-testlogs", "build", "cmake-build", ".git",
    ".cache", "__pycache__", "node_modules", ".worktrees", ".bazel",
})

_WELL_KNOWN_TARGETS = {
    "std": "@arc//lib:std_module",
    "arc": "@arc//lib:arc_module",
}

_CLANG_SCAN_DEPS_CANDIDATES = [
    "/usr/lib/llvm-21/bin/clang-scan-deps",
    "/usr/lib/llvm-20/bin/clang-scan-deps",
    "clang-scan-deps",
]


def _find_clang_scan_deps():
    for c in _CLANG_SCAN_DEPS_CANDIDATES:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    for d in os.environ.get("PATH", "").split(os.pathsep):
        p = os.path.join(d, "clang-scan-deps")
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None


def _extract_arc_block(path):
    """Extract the content between arc-begin and arc-end from a .cpp file."""
    lines = []
    in_arc = False
    try:
        with open(path, encoding="utf-8", errors="ignore") as fh:
            for line in fh:
                if not in_arc:
                    if "arc-begin" in line:
                        in_arc = True
                else:
                    if "arc-end" in line:
                        break
                    lines.append(line)
    except OSError:
        return None
    return "".join(lines) if (lines and in_arc) else None


def _scan_with_clang_deps(scan_inputs, scan_dir):
    """Run clang-scan-deps --format=p1689 on a batch of source files."""
    if not scan_inputs:
        return {}

    binary = _find_clang_scan_deps()
    if not binary:
        return {}

    output_to_key = {}
    cmds = []
    for key, scan_path, x_flag in scan_inputs:
        out = scan_path + ".scan.o"
        output_to_key[out] = key
        cmds.append({
            "command": "clang++ -std=c++23 -x {} {}".format(x_flag, scan_path),
            "file":      scan_path,
            "directory": scan_dir,
            "output":    out,
        })

    compdb = None
    try:
        with tempfile.NamedTemporaryFile(
                suffix=".json", mode="w", delete=False) as f:
            json.dump(cmds, f)
            compdb = f.name

        result = subprocess.run(
            [binary, "--format=p1689", "-compilation-database=" + compdb],
            capture_output=True, text=True, timeout=120,
        )
    except Exception:
        return {}
    finally:
        if compdb:
            try: os.unlink(compdb)
            except OSError: pass

    if result.returncode != 0:
        return {}

    try:
        data = json.loads(result.stdout)
    except (ValueError, KeyError):
        return {}

    results = {}
    for rule in data.get("rules", []):
        if "error" in rule:
            continue

        primary_out = rule.get("primary-output", "")
        key = output_to_key.get(primary_out)
        if key is None:
            continue

        provides = rule.get("provides", [])
        deps     = [r["logical-name"] for r in rule.get("requires", [])]

        if provides:
            prov         = provides[0]
            module_name  = prov.get("logical-name", "")
            is_interface = prov.get("is-interface", False)
            results[key] = (module_name or None, deps, is_interface)
        else:
            results[key] = (None, deps, False)

    return results


def _scan_file_regex_text(content):
    module_name       = None
    is_interface_unit = False
    direct_deps       = []
    seen              = set()

    for raw_line in content.splitlines():
        s = raw_line.strip()

        if module_name is None:
            if s.startswith("export module ") and s.endswith(";"):
                module_name       = s[len("export module "):-1].strip()
                is_interface_unit = True
                continue
            if s.startswith("module ") and s.endswith(";"):
                module_name = s[len("module "):-1].strip()
                continue

        if s.startswith("export "):
            s = s[7:].strip()
        if not s.startswith("import "):
            continue
        rest = s[7:].strip()
        if not rest.endswith(";"):
            continue
        name = rest[:-1].strip()
        if not name or name[0] in ("<", '"'):
            continue

        if name.startswith(":"):
            if module_name:
                full = module_name.split(":")[0] + name
                if full not in seen:
                    direct_deps.append(full)
                    seen.add(full)
            continue

        if name not in seen:
            direct_deps.append(name)
            seen.add(name)

    return module_name, direct_deps, is_interface_unit


def _scan_file_regex(path):
    try:
        with open(path, encoding="utf-8", errors="ignore") as fh:
            content = fh.read()
    except OSError:
        return None, [], False
    return _scan_file_regex_text(content)


def _sanitize(s):
    return s.replace(".", "_").replace(":", "__")


def _find_build_package(workspace, file_abs):
    directory = os.path.dirname(file_abs)
    while True:
        if (os.path.exists(os.path.join(directory, "BUILD")) or
                os.path.exists(os.path.join(directory, "BUILD.bazel"))):
            rel = os.path.relpath(directory, workspace).replace(os.sep, "/")
            return "//" if rel == "." else "//" + rel
        parent = os.path.dirname(directory)
        if parent == directory:
            return "//"
        directory = parent


def _target_for_module(workspace, file_abs, module_name, file_type, label_prefix):
    package   = _find_build_package(workspace, file_abs)
    sanitized = _sanitize(module_name)
    if file_type == "ixx":
        target_name = "_ixx_" + sanitized
    elif file_type == "ixx_arc":
        target_name = "_arcgen_" + sanitized
    elif file_type == "cpp_embed":
        target_name = "_arcembed_" + sanitized
    else:
        target_name = "_ixx_" + sanitized
    return "{}{}:{}".format(label_prefix, package, target_name)


def _starlark_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _starlark_list(items):
    if not items:
        return "[]"
    return "[" + ", ".join(_starlark_str(i) for i in items) + "]"


def _parse_root(spec):
    """Parse '<path>|<short_prefix>|<label_prefix>'. Tolerates missing trailing fields."""
    parts = spec.split("|")
    while len(parts) < 3:
        parts.append("")
    return parts[0], parts[1], parts[2]


def main(argv):
    roots = []
    for arg in argv:
        if arg.startswith("--root="):
            roots.append(_parse_root(arg[len("--root="):]))
        else:
            # Backward compat: positional workspace arg (no prefixes).
            roots.append((arg, "", ""))

    if not roots:
        print("Usage: scan_module_deps.py --root=<path>|<short_prefix>|<label_prefix> ...",
              file=sys.stderr)
        sys.exit(1)

    module_deps             = {}
    file_deps               = {}
    module_sources          = {}
    module_provider_targets = dict(_WELL_KNOWN_TARGETS)

    # Phase 1 — collect files across all roots; extract arc-embed blocks.
    # Each entry: (key, abs_path, workspace, label_prefix,
    #              is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed)
    all_files       = []
    arc_embed_temps = {}  # key -> temp_abs_path
    seen_keys       = set()

    for workspace, short_prefix, label_prefix in roots:
        for root, dirs, files in os.walk(workspace, followlinks=False):
            dirs[:] = sorted(
                d for d in dirs
                if d not in _SKIP_DIRS and not d.startswith("bazel-")
            )
            for fname in sorted(files):
                is_ixx_arc = fname.endswith(".ixx.arc")
                is_ixx     = fname.endswith(".ixx") and not is_ixx_arc
                is_cpp_src = fname.endswith(".cpp") or fname.endswith(".cc")
                if not (is_ixx or is_ixx_arc or is_cpp_src):
                    continue

                abs_path = os.path.join(root, fname)
                rel_path = os.path.relpath(abs_path, workspace).replace(os.sep, "/")
                key      = short_prefix + rel_path

                if key in seen_keys:
                    continue
                seen_keys.add(key)

                is_arc_embed = False
                if is_cpp_src:
                    arc_content = _extract_arc_block(abs_path)
                    if arc_content is not None:
                        is_arc_embed = True
                        try:
                            tmp = tempfile.NamedTemporaryFile(
                                suffix=".ixx", mode="w", delete=False,
                                encoding="utf-8")
                            tmp.write(arc_content)
                            tmp.close()
                            arc_embed_temps[key] = tmp.name
                        except Exception:
                            pass

                all_files.append((
                    key, abs_path, workspace, label_prefix,
                    is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed,
                ))

    # Phase 2 — scan everything with clang-scan-deps in one batch.
    scan_inputs = []
    for entry in all_files:
        (key, abs_path, workspace, label_prefix,
         is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed) = entry
        if is_ixx or is_ixx_arc:
            scan_inputs.append((key, abs_path, "c++-module"))
        elif is_arc_embed:
            if key in arc_embed_temps:
                scan_inputs.append((key, arc_embed_temps[key], "c++-module"))
        else:
            scan_inputs.append((key, abs_path, "c++"))

    scan_dir = roots[0][0]
    try:
        clang_results = _scan_with_clang_deps(scan_inputs, scan_dir)
    finally:
        for tmp_path in arc_embed_temps.values():
            try: os.unlink(tmp_path)
            except OSError: pass

    # Phase 3 — process results.
    for entry in all_files:
        (key, abs_path, workspace, label_prefix,
         is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed) = entry

        if key in clang_results:
            module_name, deps, is_interface = clang_results[key]
        elif is_arc_embed:
            arc_content = _extract_arc_block(abs_path)
            if arc_content:
                module_name, deps, is_interface = _scan_file_regex_text(arc_content)
            else:
                module_name, deps, is_interface = None, [], False
        else:
            module_name, deps, is_interface = _scan_file_regex(abs_path)

        if (is_ixx_arc or is_arc_embed) and "arc" not in deps:
            deps = list(deps) + ["arc"]

        if module_name:
            is_partition = ":" in module_name
            is_impl_unit = not is_interface and not is_partition

            if is_impl_unit:
                impl_deps = ([module_name] if module_name not in deps else []) + deps
                if impl_deps:
                    file_deps[key] = impl_deps
            elif is_interface or module_name not in module_deps:
                module_deps[module_name] = deps

                if is_interface or is_partition:
                    if is_ixx_arc:
                        file_type = "ixx_arc"
                    elif is_arc_embed:
                        file_type = "cpp_embed"
                    else:
                        # .ixx, or a plain .cpp/.cc that is itself a module
                        # interface unit (e.g. a third-party module shipped as
                        # .cpp): both map to the _ixx_ provider-target name, the
                        # same convention arc_module uses for `mods`.
                        file_type = "ixx"
                    module_sources[key] = module_name
                    if (is_interface or is_partition) and module_name not in module_provider_targets:
                        target = _target_for_module(
                            workspace, abs_path, module_name, file_type, label_prefix)
                        module_provider_targets[module_name] = target

        elif deps:
            file_deps[key] = deps

    out = [
        "# Auto-generated by bazel/scan_module_deps.py — do not edit manually.",
        "",
        "MODULE_DIRECT_DEPS = {",
    ]
    for name in sorted(module_deps):
        out.append("    {}: {},".format(
            _starlark_str(name), _starlark_list(module_deps[name])))
    out += ["}", "", "FILE_DIRECT_DEPS = {"]
    for k in sorted(file_deps):
        out.append("    {}: {},".format(
            _starlark_str(k), _starlark_list(file_deps[k])))
    out += ["}", "", "MODULE_SOURCES = {"]
    for k in sorted(module_sources):
        out.append("    {}: {},".format(
            _starlark_str(k), _starlark_str(module_sources[k])))
    out += ["}", "", "MODULE_PROVIDER_TARGETS = {"]
    for name in sorted(module_provider_targets):
        out.append("    {}: {},".format(
            _starlark_str(name), _starlark_str(module_provider_targets[name])))
    out += ["}", ""]

    print("\n".join(out))


if __name__ == "__main__":
    main(sys.argv[1:])
