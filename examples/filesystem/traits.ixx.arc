export module examples.filesystem.traits;

import examples.filesystem.entry;
import std;

namespace examples::filesystem::trait {

// Path manipulation utilities
trait PathOps
{
    // normalise path (resolve . and .., remove trailing slashes, etc.)
    normalise(std::string_view path) const -> std::string

    // Get parent directory path
    parent(std::string_view path) const -> std::string

    // Get filename component
    filename(std::string_view path) const -> std::string

    // Join two path components
    join(std::string_view base, std::string_view child) const -> std::string

    // Check if path is the root
    isRoot(std::string_view path) const -> bool
}

// Low-level storage operations
trait Storage [Types]
{
    type GetResult
    type Children

    // Get entry at path, nullopt/nullptr if not found
    get(std::string_view path) const -> Types::GetResult

    // Put entry at path (overwrites if exists)
    put(std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>

    // Erase entry at path, returns true if existed
    erase(std::string_view path) -> bool

    // List immediate children of a directory path
    children(std::string_view path) const -> Types::Children
}

// High-level filesystem operations
trait Filesystem [Types]
{
    type DataView
    type Children

    // Read file contents
    read(std::string_view path) const -> std::expected<typename Types::DataView, FsError>

    // Write data to file (creates if not exists, overwrites if exists)
    write(std::string_view path, std::string data) -> std::expected<void, FsError>

    // Create directory
    mkdir(std::string_view path) -> std::expected<void, FsError>

    // Remove file or empty directory
    remove(std::string_view path) -> std::expected<void, FsError>

    // List directory contents
    list(std::string_view path) const -> std::expected<typename Types::Children, FsError>

    // Check if path exists
    exists(std::string_view path) const -> bool

    // Check if path is a directory
    isDir(std::string_view path) const -> bool
}

// Persistence to/from disk
trait DirectorySync
{
    loadFromDirectory(std::string_view path) -> std::expected<void, FsError>
    dumpToDirectory(std::string_view path) const -> std::expected<void, FsError>
}

}
