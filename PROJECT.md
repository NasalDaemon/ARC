# ARC Project

C++23 project using ARC (Architecture Realised through Code) — a framework for compile-time enforced, zero-overhead software architecture.

## Skills & tools

- **Location:** `./.agents/skills/`
- **available-skills:**
  - `arc-cmake`: ARC CMake integration, build commands, adding nodes and tests.
  - `arc-dsl`: Use when reading or writing .arc files, trait, cluster, domain, policy, protocol definitions.
  - `arc-nodes`: Use when reading or writing node implementations, planning cluster structure.
  - `arc-node-builtins`: Use when choosing higher-order wrappers (Union, Virtual, Collection, DataStore, threading, access control, Spy, keys).
  - `arc-testing`: Use when testing ARC nodes, clusters, domains.

## Build:

Configure CMake, build and test with examples and `import std` support:
```bash
./configure.bash
./build.bash
```
Once CMake is configured, build again without configuring:
```bash
./build.bash
```
