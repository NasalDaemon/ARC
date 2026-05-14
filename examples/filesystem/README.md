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
| `trait::LineReader` | REPL input with prompt and tab completion | Testable I/O abstraction |
| `trait::Commands` | REPL command dispatch | Separates I/O from business logic |
| `trait::Output` | REPL output writing | Swappable output (console, test capture) |

### The Pattern: Dependency Inversion

The `Filesystem` node **depends on traits, not implementations**:

```cpp
// In filesystem.ixx - depends on abstractions
struct Filesystem
{
    template<class Context>
    struct Node : arc::Node::
        Uses<Storage, PathOps>::
        Impl<trait::Filesystem>
    { /* ... */ };
};

// The cluster wires concrete implementations via Root type parameter
cluster Filesystem [Root]
{
    fs = node::Filesystem
    pathOps = node::PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage
}
```

This means you can swap `MemoryStorage` for `DiskStorage` by changing the Root configuration:

```cpp
struct InMemoryRoot { using FilesystemStorage = node::MemoryStorage; };
struct DiskRoot { using FilesystemStorage = node::DiskStorage; };

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
- `load <directory>` - Load filesystem from host directory
- `dump <directory>` - Dump filesystem to host directory

### Utilities
- `exists <path>` - Check if a path exists
- `help` or `?` - Show available commands
- `exit` or `quit` or `q` - Exit the REPL

### Interactive Features
- **Tab Completion**: Auto-complete file and directory paths
- **Command History**: Navigate with ↑/↓ arrow keys
- **Cursor Movement**: Move cursor left/right with ←/→ arrow keys for editing

## Example Usage

```
In-Memory Filesystem REPL
Commands: ls, cat, write, mkdir, rm, tree, exists, load, dump, help, exit
Navigation: ↑/↓ history, ←/→ cursor, Backspace/Delete edit

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
arc::test::Graph<node::PathOps> graph;
auto pathOps = graph.node.asTrait(trait::pathOps);
CHECK(pathOps.normalise("/a/../b") == "/b");

// Test Filesystem with mocked dependencies
struct MockStorageTypes {
    using GetResult = Entry const*;
    using Children = std::vector<std::string_view>;
};

arc::test::Graph<node::Filesystem, arc::test::Mock<MockStorageTypes>> graph;
auto fs = graph.asTrait(trait::filesystem);

// Define mock behavior for Storage and PathOps traits
graph.mocks->setThrowIfMissing();
MockStorage storage(graph);
graph.mocks->define(
    [](trait::PathOps::normalise, std::string_view path) {
        return std::string(path);
    },
    [](trait::PathOps::parent, std::string_view path) {
        auto pos = path.rfind('/');
        return pos == 0 || pos == std::string_view::npos
            ? "/" : std::string(path.substr(0, pos));
    },
    [](trait::PathOps::isRoot, std::string_view path) {
        return path == "/";
    }
);

CHECK(fs.mkdir("/test").has_value());
```

See [`tests/`](./tests/) for comprehensive examples of testing at each layer.

### Pattern 2: Nexus Nodes Coordinate Dependencies

The `Filesystem` node acts as a **nexus** that orchestrates lower-level components without exposing their details:

```cpp
// In filesystem.impl.ixx - Filesystem coordinates PathOps and Storage
FILESYSTEM::write(std::string_view path, std::string data)
    -> std::expected<void, FsError>
{
    auto pathOps = getPathOps();
    auto storage = getStorage();

    std::string normalised = pathOps.normalise(path);  // Delegate to PathOps
    // ... validation logic ...
    return storage.put(normalised, Entry::file(std::move(data)));  // Delegate to Storage
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

The cluster definition in [`src/clusters.ixx.arc`](./src/clusters.ixx.arc) is a **complete, readable specification** of how components connect:

```arc
cluster Filesystem [Root]
{
    fs = node::Filesystem
    pathOps = node::PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage

    [trait::DirectorySync]
    .. --> storage
}

cluster Repl
{
    repl = node::Repl
    lineReader = node::TerminalLineReader
    commands = node::CommandHandler
    output = node::ConsoleOutput
    fs = cluster::Filesystem

    [trait::LineReader]    repl --> lineReader
    [trait::Commands]      repl --> commands
    [trait::Output]        repl --> output

    [trait::Filesystem]    lineReader --> fs
                           commands   --> fs
    [trait::DirectorySync] commands   --> fs
}
```

This makes the architecture **visible and refactorable**. The `LineReader` depends on `Filesystem` for tab completion (querying the filesystem during input), while the `Repl` orchestrates without touching the filesystem directly.

## Files

- [`src/in_memory_repl.cpp`](./src/in_memory_repl.cpp) - In-memory filesystem REPL entry point
- [`src/disk_repl.cpp`](./src/disk_repl.cpp) - Disk-backed filesystem REPL entry point
- [`src/clusters.ixx.arc`](./src/clusters.ixx.arc) - Cluster definitions and component wiring
- [`src/graphs.ixx`](./src/graphs.ixx) - Graph type aliases for different storage backends
- [`src/traits.ixx.arc`](./src/traits.ixx.arc) - Trait definitions with `pre`/`post` contracts
- [`src/types.ixx`](./src/types.ixx) - Filesystem entry types (`Entry`, `DiskEntry`) and error handling
- [`src/nodes/filesystem.ixx`](./src/nodes/filesystem.ixx) - Filesystem nexus coordinator node
- [`src/nodes/memory_storage.ixx`](./src/nodes/memory_storage.ixx) - In-memory storage backend
- [`src/nodes/disk_storage.ixx`](./src/nodes/disk_storage.ixx) - Disk-based storage backend
- [`src/nodes/path_ops.ixx`](./src/nodes/path_ops.ixx) - Path manipulation utilities
- [`src/nodes/terminal_line_reader.ixx`](./src/nodes/terminal_line_reader.ixx) - Terminal input with history and cursor support
- [`src/nodes/command_handler.ixx`](./src/nodes/command_handler.ixx) - REPL command dispatch
- [`src/nodes/console_output.ixx`](./src/nodes/console_output.ixx) - Console output implementation
- [`src/nodes/repl.ixx`](./src/nodes/repl.ixx) - Interactive REPL node
- [`tests/`](./tests/) - BDD-style unit and integration tests

## Testing

Tests use BDD-style macros (`SCENARIO`/`GIVEN`/`WHEN`/`THEN`). Every trait boundary is tested in isolation with mocks, and integration tests verify the full stack.

Run the test suite:

```bash
cmake --preset conan-default -DARC_BUILD_EXAMPLES=ON -DARC_BUILD_TESTS=ON
cmake --build build --target arc_example_filesystem_tests
./build/examples/filesystem/Release/arc_example_filesystem_tests
```

## What You'll Learn

| Concept | Where to Look | Key Takeaway |
|---------|---------------|---------------|
| **Trait Contracts** | [`src/traits.ixx.arc`](./src/traits.ixx.arc) | `pre`/`post` contracts with `nonEmpty()` helper |
| **Node Implementation** | [`src/nodes/*.ixx`](./src/nodes/) | Direct method names matching trait signatures |
| **Cluster Wiring** | [`src/clusters.ixx.arc`](./src/clusters.ixx.arc) | Declarative dependency injection with Root parameters |
| **Swappable Backends** | [`src/graphs.ixx`](./src/graphs.ixx) | Parameterizing clusters with Root types |
| **BDD Testing** | [`tests/`](./tests/) | `SCENARIO`/`GIVEN`/`WHEN`/`THEN` with mocks |
| **Nexus Pattern** | [`src/nodes/filesystem.ixx`](./src/nodes/filesystem.ixx) | Coordinating multiple dependencies |
| **Domain-Driven I/O** | [`src/nodes/terminal_line_reader.ixx`](./src/nodes/terminal_line_reader.ixx) | LineReader queries Filesystem directly for tab completion |
| **Testable REPL** | [`tests/test_repl.cpp`](./tests/test_repl.cpp) | Mock `LineReader` to test REPL without terminal |

## Related Documentation

- [ARC Framework Overview](../../README.md)
- [Trait System Guide](../../docs/trait-syntax.md)
- [Cluster Syntax](../../docs/cluster-syntax.md)
