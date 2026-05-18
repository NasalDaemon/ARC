#!/usr/bin/env python3
"""List source files that affect the module dependency graph.

Prints absolute paths (one per line) for:
  - All .ixx files (module interface units)
  - All .ixx.arc files (ARC DSL module sources)
  - All .cpp/.cc files that contain arc-begin blocks (arc_embed_module sources)

Accepts one or more --root=<absolute_path> arguments. Each root is walked.

Used by module_scan.bzl to register precise file watches so that the module
dependency scan only re-runs when a module-affecting file changes, not on
every .cpp implementation file edit.

Usage: python3 list_module_files.py --root=<workspace_root> [--root=<other_root> ...]
"""

import os
import sys

_SKIP_DIRS = frozenset({
    "bazel-bin", "bazel-out", "bazel-testlogs", "build", "cmake-build", ".git",
    ".cache", "__pycache__", "node_modules", ".worktrees", ".bazel",
})


def walk_root(workspace):
    for root, dirs, files in os.walk(workspace, followlinks=False):
        dirs[:] = sorted(
            d for d in dirs
            if d not in _SKIP_DIRS and not d.startswith("bazel-")
        )
        for fname in files:
            path = os.path.join(root, fname)
            if fname.endswith(".ixx") or fname.endswith(".ixx.arc"):
                print(path)
            elif fname.endswith(".cpp") or fname.endswith(".cc"):
                try:
                    with open(path, encoding="utf-8", errors="ignore") as fh:
                        if "arc-begin" in fh.read():
                            print(path)
                except OSError:
                    pass


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
