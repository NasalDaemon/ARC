export module examples.filesystem.traits;

import examples.filesystem.types;
import std;

namespace examples::filesystem {

// Path manipulation utilities
trait PathOps
{
    // normalise path (resolve . and .., remove trailing slashes, etc.)
    normalise(std::string_view path) const -> std::string
        pre(nonEmpty(path))

    // Get parent directory path
    parent(std::string_view path) const -> std::string
        pre(nonEmpty(path))

    // Get filename component
    filename(std::string_view path) const -> std::string
        pre(nonEmpty(path))

    // Join two path components
    join(std::string_view base, std::string_view child) const -> std::string
        pre(nonEmpty(base))

    // Check if path is the root
    isRoot(std::string_view path) const -> bool
        pre(nonEmpty(path))
}

// Low-level storage operations
trait Storage [Types]
{
    type GetResult
    type Children

    // Get entry at path, nullopt/nullptr if not found
    get(std::string_view path) const -> Types::GetResult
        pre(nonEmpty(path))

    // Put entry at path (overwrites if exists)
    put(std::string_view path, Entry entry) -> std::expected<void, FsError>
        pre(nonEmpty(path))

    // Erase entry at path, returns true if existed
    erase(std::string_view path) -> bool
        pre(nonEmpty(path))

    // List immediate children of a directory path
    children(std::string_view path) const -> Types::Children
        pre(nonEmpty(path))
}

// High-level filesystem operations
trait Filesystem [Types]
{
    type DataView
    type Children

    // Read file contents
    read(std::string_view path) const -> std::expected<typename Types::DataView, FsError>
        pre(nonEmpty(path))

    // Write data to file (creates if not exists, overwrites if exists)
    write(std::string_view path, std::string data) -> std::expected<void, FsError>
        pre(nonEmpty(path))

    // Create directory
    mkdir(std::string_view path) -> std::expected<void, FsError>
        pre(nonEmpty(path))

    // Remove file or empty directory
    remove(std::string_view path) -> std::expected<void, FsError>
        pre(nonEmpty(path))

    // List directory contents
    list(std::string_view path) const -> std::expected<typename Types::Children, FsError>
        pre(nonEmpty(path))

    // Check if path exists
    exists(std::string_view path) const -> bool
        pre(nonEmpty(path))

    // Check if path is a directory
    isDir(std::string_view path) const -> bool
        pre(nonEmpty(path))
}

// Persistence to/from disk
trait DirectorySync
{
    loadFromDirectory(std::string_view path) -> std::expected<void, FsError>
        pre(nonEmpty(path))
    dumpToDirectory(std::string_view path) const -> std::expected<void, FsError>
        pre(nonEmpty(path))
}

// REPL output
trait Output
{
    write(std::string_view text) -> void
    writeLine(std::string_view text) -> void
}

// REPL input
trait LineReader
{
    // Read a line with the given prompt; returns nullopt on EOF
    readLine(std::string_view prompt) -> std::optional<std::string>
}

// REPL command dispatch
trait Commands
{
    // Check if input is a known command
    isCommand(std::string_view input) const -> bool

    // Execute a command; returns output string or error message
    execute(std::span<std::string_view> args) -> std::expected<std::string, std::string>
        pre(not args.empty())
        pre(self.isCommand(args[0]))
}

}
