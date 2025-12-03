# ARC Filesystem Example

This example demonstrates **good architectural patterns enabled by ARC** through a filesystem implementation with swappable storage backends. It showcases how ARC's trait-based dependency injection creates clean, testable, and maintainable code.

## Why This Example?

The filesystem domain is ideal for demonstrating ARC patterns because it naturally decomposes into:

- **Layered Abstractions**: High-level operations built on low-level primitives
- **Swappable Implementations**: Storage backends can be changed without touching business logic
- **Clear Interfaces**: Each component has well-defined responsibilities
- **Testable Contracts**: Every trait boundary is a natural testing seam

## Architecture: Separation of Concerns

ARC enables clean separation through its trait system:

### The Pattern: Interface Segregation

Each trait defines a **single, focused responsibility**:

| Trait | Responsibility | Why Separate? |
|-------|---------------|---------------|
| `trait::Filesystem` | High-level operations (read, write, mkdir) | Business logic layer |
| `trait::PathOps` | Path normalization and manipulation | Reusable across storage backends |
| `trait::Storage` | Low-level data access (get, put, erase) | Swappable storage implementations |
| `trait::DirectorySync` | Load/dump filesystem to host directory | Persistence operations |

### The Pattern: Dependency Inversion

The `Filesystem` node **depends on traits, not implementations**:

```cpp
// In filesystem.ixx - depends on abstractions
using Depends = arc::Depends<trait::Storage, trait::PathOps>;

// The cluster wires concrete implementations via Root type parameter
cluster Filesystem [Root]
{
    fs = filesystem::Filesystem
    pathOps = PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage
}
```

This means you can swap `MemoryStorage` for `DiskStorage` by changing the Root configuration:

```cpp
struct InMemoryRoot { using FilesystemStorage = MemoryStorage; };
struct DiskRoot { using FilesystemStorage = DiskStorage; };

using InMemory = arc::Graph<cluster::Filesystem, InMemoryRoot>;
using Disk = arc::Graph<cluster::Filesystem, DiskRoot>;
```

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

### Running the Examples

Two REPL executables are provided with different storage backends:

```bash
# In-memory filesystem (data lost on exit)
./build/examples/filesystem/Release/arc_example_filesystem_in_memory_repl

# Disk-backed filesystem (operates on real files)
./build/examples/filesystem/Release/arc_example_filesystem_disk_repl
```

The disk REPL operates directly on the host filesystem, while the in-memory REPL starts with an empty virtual filesystem.

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
- `load <directory>` - Load filesystem from host directory (in-memory only)
- `dump <directory>` - Dump filesystem to host directory

### Utilities
- `exists <path>` - Check if a path exists
- `help` or `?` - Show available commands
- `exit` or `quit` or `q` - Exit the REPL

### Interactive Features
- **Tab Completion**: Auto-complete file and directory paths
- **Command History**: Navigate with ↑/↓ arrow keys
- **Cursor Movement**: Move cursor left/right with ←/→ arrow keys for editing
- **Path Completion**: Shows common prefix and ambiguous completions

## Example Usage

```
In-Memory Filesystem REPL
Commands: ls [path], cat <path>, write <path> <content>, mkdir <path>, rm <path>, tree [path], load <dir>, dump <dir>, help, exit
Navigation: ↑/↓ history, ←/→ cursor, Backspace/Delete edit, Tab completion

> mkdir /docs
> mkdir /docs/api
> write /docs/readme.txt Welcome to the filesystem example
> write /docs/api/overview.txt This demonstrates ARC's trait system
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
arc::test::Graph<PathOps> graph;
CHECK(graph.asTrait(trait::pathOps).normalise("/a/../b") == "/b");

// Test Filesystem with mocked dependencies
struct MockStorageTypes {
    using GetResult = InMemoryEntry const*;
    using Children = std::vector<std::string_view>;
};

arc::test::Graph<Filesystem, arc::test::Mock<MockStorageTypes>> graph;

// Define mock behavior for Storage and PathOps traits
graph.mocks->define(
    [&](trait::Storage::get, std::string_view path) -> InMemoryEntry const* {
        return storage.get(path);
    },
    [&](trait::Storage::put, std::string_view path, InMemoryEntry entry) {
        storage.put(path, std::move(entry));
    }
    // ... other trait methods
);

CHECK(graph.asTrait(trait::filesystem).mkdir("/test").has_value());
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
    storage.put(normalised, InMemoryEntry::file(std::move(data)));  // Delegate to Storage
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

The cluster definition in [`clusters.ixx.arc`](./clusters.ixx.arc) is a **complete, readable specification** of how components connect:

```arc
cluster Filesystem [Root]
{
    fs = filesystem::Filesystem
    pathOps = PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage

    [trait::DirectorySync]
    .. --> storage
}

cluster Repl
{
    repl = filesystem::Repl
    fs = Filesystem

    [trait::Filesystem]    repl --> fs
    [trait::DirectorySync] repl --> fs
}
```

This makes the architecture **visible and refactorable**.

## Files

- [`in_memory_repl.cpp`](./in_memory_repl.cpp) - In-memory filesystem REPL entry point
- [`disk_repl.cpp`](./disk_repl.cpp) - Disk-backed filesystem REPL entry point
- [`clusters.ixx.arc`](./clusters.ixx.arc) - Cluster definitions and component wiring
- [`graphs.ixx`](./graphs.ixx) - Graph type aliases for different storage backends
- [`traits.ixx.arc`](./traits.ixx.arc) - Trait definitions for filesystem interfaces
- [`entry.ixx`](./entry.ixx) - Filesystem entry types (`InMemoryEntry`, `DiskEntry`) and error handling
- [`nodes/filesystem.ixx`](./nodes/filesystem.ixx) - Filesystem nexus coordinator node
- [`nodes/memory_storage.ixx`](./nodes/memory_storage.ixx) - In-memory storage backend
- [`nodes/disk_storage.ixx`](./nodes/disk_storage.ixx) - Disk-based storage backend
- [`nodes/path_ops.ixx`](./nodes/path_ops.ixx) - Path manipulation utilities
- [`nodes/repl.ixx`](./nodes/repl.ixx) - Interactive REPL node
- [`tests/`](./tests/) - Unit tests for all components

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
| **Cluster Wiring** | [`clusters.ixx.arc`](./clusters.ixx.arc) | Declarative dependency injection |
| **Swappable Backends** | [`graphs.ixx`](./graphs.ixx) | Parameterizing clusters with Root types |
| **Layered Testing** | [`tests/`](./tests/) | Testing each layer independently |
| **Nexus Pattern** | [`nodes/filesystem.ixx`](./nodes/filesystem.ixx) | Coordinating multiple dependencies |

## Related Documentation

- [ARC Framework Overview](../../README.md)
- [Trait System Guide](../../docs/trait-syntax.md)
- [Cluster Syntax](../../docs/cluster-syntax.md)
