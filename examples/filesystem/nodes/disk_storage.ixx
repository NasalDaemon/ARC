export module examples.filesystem.disk_storage;

import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// DiskStorage node reads and writes directly to the filesystem under a root path
export struct DiskStorage : arc::NodeImpl<Storage, DirectorySync>
{
    struct Types
    {
        using GetResult = std::optional<DiskEntry>;
        using Children = std::vector<std::string>;
    };

    DiskStorage();

    auto get(std::string_view path) const -> std::optional<DiskEntry>;
    auto put(std::string_view path, Entry entry) -> std::expected<void, FsError>;
    auto erase(std::string_view path) -> bool;
    auto children(std::string_view path) const -> std::vector<std::string>;

    auto loadFromDirectory(std::string_view directory) -> std::expected<void, FsError>;
    auto dumpToDirectory(std::string_view directory) const -> std::expected<void, FsError>;

private:
    std::filesystem::path rootPath;

    auto toAbsolute(std::string_view path) const -> std::filesystem::path;
};

} // namespace examples::filesystem::node
