export module examples.filesystem.memory_storage;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

struct hash_str : std::hash<std::string_view>, std::hash<std::string>
{
    using is_transparent = void;
    using std::hash<std::string_view>::operator();
    using std::hash<std::string>::operator();
};

// MemoryStorage node holds the filesystem tree in memory
export struct MemoryStorage : arc::NodeImpl<Storage*, DirectorySync*>
{
    struct Types
    {
        using GetResult = InMemoryEntry const*;
        using Children = std::vector<std::string_view>;
    };

    MemoryStorage();

    auto impl(Storage::get, std::string_view path) const -> InMemoryEntry const*;
    auto impl(Storage::put, std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>;
    auto impl(Storage::erase, std::string_view path) -> bool;
    auto impl(Storage::children, std::string_view path) const -> std::vector<std::string_view>;

    auto impl(DirectorySync::loadFromDirectory, std::string_view directory) -> std::expected<void, FsError>;
    auto impl(DirectorySync::dumpToDirectory, std::string_view directory) const -> std::expected<void, FsError>;

private:
    // Full path -> Entry mapping
    std::unordered_map<std::string, InMemoryEntry, hash_str, std::equal_to<>> entries;
    std::unordered_map<std::string, std::vector<std::pair<std::string const, InMemoryEntry>*>, hash_str, std::equal_to<>> children;

    // Already normalised path
    static std::string_view parent(std::string_view path);
};

} // namespace examples::filesystem::node
