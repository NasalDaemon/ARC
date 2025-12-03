export module examples.filesystem.memory_storage;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem {

struct hash_str : std::hash<std::string_view>, std::hash<std::string>
{
    using is_transparent = void;
    using std::hash<std::string_view>::operator();
    using std::hash<std::string>::operator();
};

// MemoryStorage node holds the filesystem tree in memory
export struct MemoryStorage : arc::Node
{
    using Traits = arc::Traits<MemoryStorage
        , trait::Storage
        , trait::DirectorySync
    >;

    struct Types
    {
        using GetResult = InMemoryEntry const*;
        using Children = std::vector<std::string_view>;
    };

    MemoryStorage();

    auto impl(trait::Storage::get, std::string_view path) const -> InMemoryEntry const*;
    auto impl(trait::Storage::put, std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>;
    auto impl(trait::Storage::erase, std::string_view path) -> bool;
    auto impl(trait::Storage::children, std::string_view path) const -> std::vector<std::string_view>;

    // Not part of trait::Storage, but useful for persistence
    auto impl(trait::DirectorySync::loadFromDirectory, std::string_view directory) -> std::expected<void, FsError>;
    auto impl(trait::DirectorySync::dumpToDirectory, std::string_view directory) const -> std::expected<void, FsError>;

private:
    // Full path -> Entry mapping
    std::unordered_map<std::string, InMemoryEntry, hash_str, std::equal_to<>> entries;
    std::unordered_map<std::string, std::vector<std::pair<std::string const, InMemoryEntry>*>, hash_str, std::equal_to<>> children;

    // Already normalised path
    static std::string_view parent(std::string_view path);
};

}
