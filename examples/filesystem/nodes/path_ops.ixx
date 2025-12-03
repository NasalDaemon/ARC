export module examples.filesystem.path_ops;

import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem {

// PathOps node handles path parsing and manipulation
export struct PathOps : arc::Node
{
    using Traits = arc::Traits<PathOps, trait::PathOps>;

    auto impl(trait::PathOps::normalise, std::string_view path) const -> std::string
    {
        return normalise(path);
    }
    auto impl(trait::PathOps::parent, std::string_view path) const -> std::string;
    auto impl(trait::PathOps::filename, std::string_view path) const -> std::string;
    auto impl(trait::PathOps::join, std::string_view base, std::string_view child) const -> std::string;
    auto impl(trait::PathOps::isRoot, std::string_view path) const -> bool;

    std::string normalise(std::string_view path) const;
};

}
