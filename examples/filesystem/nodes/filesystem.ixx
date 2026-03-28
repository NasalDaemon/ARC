export module examples.filesystem.filesystem;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// Filesystem node orchestrates Storage and PathOps to provide high-level operations
export struct Filesystem
{
    template<class Context>
    struct Node : arc::Node::
        Uses<Storage, PathOps>::
        Impl<trait::Filesystem>
    {
        struct Types
        {
            using StorageTypes = arc::ResolveTypes<Node, Storage>;
            using GetResult = StorageTypes::GetResult;
            using Children = StorageTypes::Children;
            // Extract DataView from the entry type returned by get()
            // GetResult is either T* or std::optional<T>
            using DataView = std::remove_cvref_t<decltype(*std::declval<GetResult>())>::DataView;
        };

        using DataView = Types::DataView;
        using Children = Types::Children;

        auto read(std::string_view path) const -> std::expected<DataView, FsError>;
        auto write(std::string_view path, std::string data) -> std::expected<void, FsError>;
        auto mkdir(std::string_view path) -> std::expected<void, FsError>;
        auto remove(std::string_view path) -> std::expected<void, FsError>;
        auto list(std::string_view path) const -> std::expected<Children, FsError>;
        auto exists(std::string_view path) const -> bool;
        auto isDir(std::string_view path) const -> bool;
    };
};

} // namespace examples::filesystem::node
