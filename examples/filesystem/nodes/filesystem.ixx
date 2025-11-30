export module examples.filesystem.filesystem;

import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem {

// Filesystem node orchestrates Storage and PathOps to provide high-level operations
export struct Filesystem
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Storage, trait::PathOps>;
        using Traits = arc::Traits<Node, trait::Filesystem>;

        struct Types
        {
            using DataView = arc::ResolveTypes<Node, trait::Storage>::Entry::DataView;
        };

        using DataView = Types::DataView;

        auto impl(trait::Filesystem::read, std::string_view path) const -> std::expected<DataView, FsError>;
        auto impl(trait::Filesystem::write, std::string_view path, std::string data) -> std::expected<void, FsError>;
        auto impl(trait::Filesystem::mkdir, std::string_view path) -> std::expected<void, FsError>;
        auto impl(trait::Filesystem::remove, std::string_view path) -> std::expected<void, FsError>;
        auto impl(trait::Filesystem::list, std::string_view path) const -> std::expected<std::vector<std::string_view>, FsError>;
        auto impl(trait::Filesystem::exists, std::string_view path) const -> bool;
        auto impl(trait::Filesystem::isDir, std::string_view path) const -> bool;
    };
};

}
