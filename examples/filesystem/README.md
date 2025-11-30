# ARC Filesystem Example

This example demonstrates **good architectural patterns enabled by ARC** through an in-memory filesystem implementation. It showcases how ARC's trait-based dependency injection creates clean, testable, and maintainable code.

## Why This Example?

The filesystem domain is ideal for demonstrating ARC patterns because it naturally decomposes into:

- **Layered Abstractions**: High-level operations built on low-level primitives
- **Swappable Implementations**: Storage backends can be changed without touching business logic
- **Clear Interfaces**: Each component has well-defined responsibilities
- **Testable Contracts**: Every trait boundary is a natural testing seam

## Architecture: Separation of Concerns

ARC enables clean separation through its trait system:

```
> FilesystemDomain <
└──── Filesystem ──┬── PathOps
                   └── MemoryStorage
```

### The Pattern: Interface Segregation

Each trait defines a **single, focused responsibility**:

| Trait | Responsibility | Why Separate? |
|-------|---------------|---------------|
| `trait::Filesystem` | High-level operations (read, write, mkdir) | Business logic layer |
| `trait::PathOps` | Path normalization and manipulation | Reusable across storage backends |
| `trait::Storage` | Low-level data access (get, put, erase) | Swappable storage implementations |

### The Pattern: Dependency Inversion

The `Filesystem` node **depends on traits, not implementations**:

```cpp
// In filesystem.ixx - depends on abstractions
using Depends = arc::Depends<trait::Storage, trait::PathOps>;

// The domain wires concrete implementations
[trait::PathOps] fs --> pathOps
[trait::Storage] fs --> Storage
```

This means you can swap `MemoryStorage` for `DiskStorage` or `NetworkStorage` without changing `Filesystem`.

## Building and Running

### Prerequisites

- C++23 compiler with modules support
- CMake 4.0+
- Conan package manager

### Build Instructions

```bash
# Configure with Conan
conan install . --output-folder=build --build=missing --profile=conanprofile.txt

# Configure CMake
cmake --preset conan-default -DARC_BUILD_EXAMPLES=ON

# Build
cmake --build build --config Release
```

### Running the Example

```bash
# Run the REPL
./build/examples/filesystem/Release/arc_example_filesystem
```

## REPL Commands

The interactive REPL supports the following commands:

### File Operations
- `ls [path]` - List directory contents (default: root "/")
- `cat <path>` - Display file contents
- `write <path> <content>` - Write content to a file
- `mkdir <path>` - Create a directory
- `rm <path>` - Remove a file or empty directory

### Navigation
- `tree [path]` - Display directory tree structure
- `load <directory>` - Load filesystem from host directory

### Utilities
- `exists <path>` - Check if a path exists
- `help` or `?` - Show available commands
- `exit` or `quit` - Exit the REPL

### Interactive Features
- **Tab Completion**: Auto-complete file and directory paths
- **Command History**: Navigate with ↑/↓ arrow keys
- **Path Completion**: Shows common prefix and ambiguous completions

## Example Usage

```
In-Memory Filesystem REPL
Commands: ls [path], cat <path>, write <path> <content>, mkdir <path>, rm <path>, tree [path], load <dir>, dump <dir>, help, exit

> mkdir /docs
> mkdir /docs/api
> write /docs/readme.txt "Welcome to the filesystem example"
> write /docs/api/overview.txt "This demonstrates ARC's trait system"
> tree
└── docs/
    ├── api/
    │   └── overview.txt
    └── readme.txt
> cat /docs/readme.txt
Welcome to the filesystem example
> ls /docs
api/
readme.txt
> exit
```

## ARC Patterns Demonstrated

### Pattern 1: Testability Through Trait Boundaries

Every trait boundary is a **natural testing seam**:

```cpp
// Test PathOps in isolation
PathOps pathOps;
CHECK(pathOps.normalise("/a/../b") == "/b");

// Test Filesystem with mock storage
arc::test::Graph<Filesystem> graph;  // Uses mock storage/path ops
auto fs = graph.asTrait(trait::filesystem);
CHECK(fs.mkdir("/test").has_value());
```

See [`tests/`](./tests/) for comprehensive examples of testing at each layer.

### Pattern 2: Nexus Nodes Coordinate Dependencies

The `Filesystem` node acts as a **nexus** that orchestrates lower-level components without exposing their details:

```cpp
// In filesystem.impl.ixx - Filesystem coordinates PathOps and Storage
auto impl(trait::Filesystem::write, std::string_view path, std::string data)
    -> std::expected<void, FsError>
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);  // Delegate to PathOps
    // ... validation logic ...
    storage.put(normalised, Entry::file(std::move(data)));  // Delegate to Storage
    return {};
}
```

This keeps business logic in the nexus while storage and path handling remain reusable, focused components.

### Pattern 3: Layered Responsibility

Each layer has a **clear, single purpose**:

| Layer | Component | Knows About | Doesn't Know About |
|-------|-----------|-------------|-------------------|
| **High** | `Filesystem` | Paths, entries, errors | How storage works internally |
| **Mid** | `PathOps` | Path syntax only | Files, directories, storage |
| **Low** | `MemoryStorage` | Key-value storage | Paths, normalization, errors |

This makes each component **independently evolvable**. You could replace `MemoryStorage` with `RedisStorage` without touching `Filesystem` or `PathOps`.

### Pattern 4: Declarative Wiring

The domain definition in [`domain.ixx.arc`](./domain.ixx.arc) is a **complete, readable specification** of how components connect:

```arc
domain FilesystemDomain
{
    fs = Filesystem
    pathOps = PathOps
    Storage = MemoryStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> Storage
}
```

This makes the architecture **visible and refactorable**.

## Files

- [`main.cpp`](./main.cpp) - REPL implementation with interactive features
- [`domain.ixx.arc`](./domain.ixx.arc) - Domain definition and component wiring
- [`traits.ixx.arc`](./traits.ixx.arc) - Trait definitions for filesystem interfaces
- [`entry.ixx`](./entry.ixx) - Filesystem entry types and error handling
- [`nodes/filesystem.ixx`](./nodes/filesystem.ixx) - Filesystem nexus coordinator node
- [`nodes/memory_storage.ixx`](./nodes/memory_storage.ixx) - In-memory storage backend
- [`nodes/path_ops.ixx`](./nodes/path_ops.ixx) - Path manipulation utilities
- [`tests/`](./tests/) - Unit tests for all components and the domain

## Testing

Run the test suite:

```bash
cmake --preset conan-default -DARC_BUILD_EXAMPLES=ON -DARC_BUILD_TESTS=ON
cmake --build build --target arc_example_filesystem_tests
./build/examples/filesystem/Release/arc_example_filesystem_tests
```

## What You'll Learn

| Concept | Where to Look | Key Takeaway |
|---------|---------------|---------------|
| **Trait Definition** | [`traits.ixx.arc`](./traits.ixx.arc) | How to define focused, composable interfaces |
| **Node Implementation** | [`nodes/*.ixx`](./nodes/) | The `impl()` pattern for trait methods |
| **Domain Wiring** | [`domain.ixx.arc`](./domain.ixx.arc) | Declarative dependency injection |
| **Layered Testing** | [`tests/`](./tests/) | Testing each layer independently |
| **Nexus Pattern** | [`nodes/filesystem.ixx`](./nodes/filesystem.ixx) | Coordinating multiple dependencies |

## Related Documentation

- [ARC Framework Overview](../../README.md)
- [Trait System Guide](../docs/trait-syntax.md)
- [Domain Syntax](../docs/domain-syntax.md)
- [Filesystem Module](../docs/arc-files.md)
