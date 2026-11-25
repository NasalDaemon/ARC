#!/usr/bin/env python3
"""List the paths the module dependency scan reads.

Prints absolute paths (one per line) for every file scan_module_deps.py scans
(.ixx, .ixx.arc, .cpp, .cc) and every directory it walks, so that a changed
import in any of them, or a source added or removed, re-runs the scan.

Accepts one or more --root=<absolute_path> arguments. Each root is walked.

Used by module_scan.bzl to register its watches. A plain .cpp is listed too:
its imports land in FILE_DIRECT_DEPS, so an import added to a test or an
implementation unit must reach the PCM set its compile action is given. When
the scan's output is unchanged, Bazel prunes the re-run and nothing is
re-analysed.

Usage: python3 list_module_files.py --root=<workspace_root> [--root=<other_root> ...]
"""

import os
import sys

_SKIP_DIRS = frozenset({
    "bazel-bin", "bazel-out", "bazel-testlogs", "build", ".git",
    ".cache", "__pycache__", "node_modules", ".worktrees", ".bazel",
})
# Build trees skipped by prefix, as in scan_module_deps.py: cmake-build-debug
# (CLion's default) and friends hold FetchContent sources.
_SKIP_DIR_PREFIXES = ("bazel-", "cmake-build")


def walk_root(workspace):
    for root, dirs, files in os.walk(workspace, followlinks=False):
        dirs[:] = sorted(
            d for d in dirs
            if d not in _SKIP_DIRS and not d.startswith(_SKIP_DIR_PREFIXES)
        )
        print(root)
        for fname in files:
            if fname.endswith((".ixx", ".ixx.arc", ".cpp", ".cc")):
                print(os.path.join(root, fname))


def main(argv):
    roots = []
    for arg in argv:
        if arg.startswith("--root="):
            roots.append(arg[len("--root="):])
        else:
            # Backward compat: positional workspace arg.
            roots.append(arg)
    if not roots:
        print("Usage: list_module_files.py --root=<workspace_root> [--root=...]",
              file=sys.stderr)
        sys.exit(1)
    for r in roots:
        walk_root(r)


if __name__ == "__main__":
    main(sys.argv[1:])
