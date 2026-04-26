#!/usr/bin/env python3
"""Scan C++20 module source files to build a precise dependency map.

Walks the workspace scanning:
  - .ixx files  (C++ module interface units)
  - .ixx.arc    (ARC DSL — valid C++20 module preamble, 'import arc;' injected)
  - .cpp / .cc  (consumers, implementation units, and arc-embed module sources)

For arc-embed .cpp files the arc block (between arc-begin / arc-end markers) is
extracted to a temporary .ixx file so that clang-scan-deps can scan it as a
module interface unit.  'import arc;' is then injected for arc-type files.

All files are scanned in a single clang-scan-deps --format=p1689 invocation.
The Python regex scanner is kept only as a fallback for files where
clang-scan-deps returns an error.

Output: a Starlark .bzl file printed to stdout containing:
  MODULE_DIRECT_DEPS      — {module_name: [dep_module_names, ...]}
  FILE_DIRECT_DEPS        — {workspace_rel_path: [dep_module_names, ...]}
  MODULE_SOURCES          — {workspace_rel_path: module_name}
  MODULE_PROVIDER_TARGETS — {module_name: bazel_target_label}
"""

import json
import os
import subprocess
import sys
import tempfile

_SKIP_DIRS = frozenset({
    "bazel-bin", "bazel-out", "bazel-testlogs", "build", "cmake-build", ".git",
    ".cache", "__pycache__", "node_modules"
})

_WELL_KNOWN_TARGETS = {
    "std": "//lib:std_module",
    "arc": "//lib:arc_module",
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
    """Extract the content between arc-begin and arc-end from a .cpp file.

    The extracted block is the embedded module source (valid C++20 module
    preamble) that the ARC generator would otherwise produce.

    Returns the extracted string, or None if no arc-begin marker is found.
    """
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


def _scan_with_clang_deps(scan_inputs, workspace):
    """Run clang-scan-deps --format=p1689 on a batch of source files.

    Args:
        scan_inputs: list of (rel_path, scan_path, x_flag)
                     x_flag is "c++-module" for module providers,
                     "c++" for consumer / implementation-unit .cpp files.
        workspace:   absolute path to the workspace root.

    Returns:
        dict {rel_path: (module_name_or_None, [dep_module_names], is_interface)}
        Empty dict on total failure; individual file errors fall through to
        the Python regex fallback in Phase 3.
    """
    if not scan_inputs:
        return {}

    binary = _find_clang_scan_deps()
    if not binary:
        return {}

    output_to_rel = {}
    cmds = []
    for rel_path, scan_path, x_flag in scan_inputs:
        out = scan_path + ".scan.o"
        output_to_rel[out] = rel_path
        cmds.append({
            "command": "clang++ -std=c++23 -x {} {}".format(
                x_flag, scan_path),
            "file":      scan_path,
            "directory": workspace,
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
            continue  # individual file error — fallback to Python regex in Phase 3

        primary_out = rule.get("primary-output", "")
        rel_path    = output_to_rel.get(primary_out)
        if rel_path is None:
            continue

        provides = rule.get("provides", [])
        deps     = [r["logical-name"] for r in rule.get("requires", [])]

        if provides:
            prov         = provides[0]
            module_name  = prov.get("logical-name", "")
            is_interface = prov.get("is-interface", False)
            results[rel_path] = (module_name or None, deps, is_interface)
        else:
            # Consumer or implementation unit without visible export module
            results[rel_path] = (None, deps, False)

    return results


# ---------------------------------------------------------------------------
# Python regex fallback — for files where clang-scan-deps returns an error
# ---------------------------------------------------------------------------

def _scan_file_regex_text(content):
    """Minimal regex scan on already-loaded text."""
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
    """Minimal regex scan: find module declaration and import lines."""
    try:
        with open(path, encoding="utf-8", errors="ignore") as fh:
            content = fh.read()
    except OSError:
        return None, [], False
    return _scan_file_regex_text(content)


# ---------------------------------------------------------------------------
# Target derivation helpers
# ---------------------------------------------------------------------------

def _sanitize(s):
    return s.replace(".", "_").replace(":", "__")


def _find_build_package(workspace, file_path):
    directory = os.path.dirname(file_path)
    while True:
        if (os.path.exists(os.path.join(directory, "BUILD")) or
                os.path.exists(os.path.join(directory, "BUILD.bazel"))):
            rel = os.path.relpath(directory, workspace).replace(os.sep, "/")
            return "//" if rel == "." else "//" + rel
        parent = os.path.dirname(directory)
        if parent == directory:
            return "//"
        directory = parent


def _target_for_module(rel_path, module_name, file_type, workspace):
    file_abs  = os.path.join(workspace, rel_path)
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
    return "{}:{}".format(package, target_name)


def _starlark_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _starlark_list(items):
    if not items:
        return "[]"
    return "[" + ", ".join(_starlark_str(i) for i in items) + "]"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(workspace):
    module_deps             = {}
    file_deps               = {}
    module_sources          = {}
    module_provider_targets = dict(_WELL_KNOWN_TARGETS)

    # Phase 1 — collect files; extract arc-embed blocks to temp .ixx files.
    all_files      = []  # (rel_path, abs_path, is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed)
    arc_embed_temps = {}  # rel_path -> temp_abs_path

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
                        arc_embed_temps[rel_path] = tmp.name
                    except Exception:
                        pass

            all_files.append(
                (rel_path, abs_path, is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed))

    # Phase 2 — scan everything with clang-scan-deps in one batch.
    # Module providers (.ixx, .ixx.arc, arc-embed extracted) → -x c++-module
    # Consumer / implementation-unit .cpp → -x c++
    scan_inputs = []
    for rel, path, is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed in all_files:
        if is_ixx or is_ixx_arc:
            scan_inputs.append((rel, path, "c++-module"))
        elif is_arc_embed:
            if rel in arc_embed_temps:
                scan_inputs.append((rel, arc_embed_temps[rel], "c++-module"))
            # original .cpp not added — we only want module-provider info here
        else:
            scan_inputs.append((rel, path, "c++"))

    try:
        clang_results = _scan_with_clang_deps(scan_inputs, workspace)
    finally:
        for tmp_path in arc_embed_temps.values():
            try: os.unlink(tmp_path)
            except OSError: pass

    # Phase 3 — process results; fall back to Python regex on clang-scan-deps misses.
    for rel_path, abs_path, is_ixx_arc, is_ixx, is_cpp_src, is_arc_embed in all_files:
        if rel_path in clang_results:
            module_name, deps, is_interface = clang_results[rel_path]
        elif is_arc_embed:
            # Regex fallback for arc-embed: only scan the arc block, not the
            # whole .cpp file (which imports its own module for testing).
            arc_content = _extract_arc_block(abs_path)
            if arc_content:
                module_name, deps, is_interface = _scan_file_regex_text(arc_content)
            else:
                module_name, deps, is_interface = None, [], False
        else:
            module_name, deps, is_interface = _scan_file_regex(abs_path)

        # .ixx.arc and arc-embed .cpp files: the ARC generator always injects
        # 'import arc;' at compile time.  Add it here so Bazel sees it.
        if (is_ixx_arc or is_arc_embed) and "arc" not in deps:
            deps = list(deps) + ["arc"]

        if module_name:
            is_partition = ":" in module_name
            is_impl_unit = not is_interface and not is_partition

            if is_impl_unit:
                impl_deps = ([module_name] if module_name not in deps else []) + deps
                if impl_deps:
                    file_deps[rel_path] = impl_deps
            elif is_interface or module_name not in module_deps:
                module_deps[module_name] = deps

                if is_interface or is_partition:
                    if is_ixx_arc:
                        file_type = "ixx_arc"
                    elif is_ixx:
                        file_type = "ixx"
                    else:
                        file_type = "cpp_embed"
                    module_sources[rel_path] = module_name
                    if (is_interface or is_partition) and module_name not in module_provider_targets:
                        target = _target_for_module(
                            rel_path, module_name, file_type, workspace)
                        module_provider_targets[module_name] = target

        elif deps:
            file_deps[rel_path] = deps

    # Emit Starlark
    out = [
        "# Auto-generated by bazel/scan_module_deps.py — do not edit manually.",
        "# Re-run: bazel run //bazel:scan_module_deps",
        "",
        "MODULE_DIRECT_DEPS = {",
    ]
    for name in sorted(module_deps):
        out.append("    {}: {},".format(
            _starlark_str(name), _starlark_list(module_deps[name])))
    out += ["}", "", "FILE_DIRECT_DEPS = {"]
    for rel in sorted(file_deps):
        out.append("    {}: {},".format(
            _starlark_str(rel), _starlark_list(file_deps[rel])))
    out += ["}", "", "MODULE_SOURCES = {"]
    for rel in sorted(module_sources):
        out.append("    {}: {},".format(
            _starlark_str(rel), _starlark_str(module_sources[rel])))
    out += ["}", "", "MODULE_PROVIDER_TARGETS = {"]
    for name in sorted(module_provider_targets):
        out.append("    {}: {},".format(
            _starlark_str(name), _starlark_str(module_provider_targets[name])))
    out += ["}", ""]

    print("\n".join(out))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: scan_module_deps.py <workspace_root>", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
