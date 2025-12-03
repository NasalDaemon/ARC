export module examples.filesystem.disk_storage;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem {

// DiskStorage node reads and writes directly to the filesystem under a root path
export struct DiskStorage : arc::Node
{
    using Traits = arc::Traits<DiskStorage
        , trait::Storage
        , trait::DirectorySync
    >;

    struct Types
    {
        using GetResult = std::optional<DiskEntry>;
        using Children = std::vector<std::string>;
    };

    DiskStorage();

    auto impl(trait::Storage::get, std::string_view path) const -> std::optional<DiskEntry>;
    auto impl(trait::Storage::put, std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>;
    auto impl(trait::Storage::erase, std::string_view path) -> bool;
    auto impl(trait::Storage::children, std::string_view path) const -> std::vector<std::string>;

    auto impl(trait::DirectorySync::loadFromDirectory, std::string_view directory) -> std::expected<void, FsError>;
    auto impl(trait::DirectorySync::dumpToDirectory, std::string_view directory) const -> std::expected<void, FsError>;

private:
    std::filesystem::path rootPath;

    auto toAbsolute(std::string_view path) const -> std::filesystem::path;
};

}
