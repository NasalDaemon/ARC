export module examples.filesystem.path_ops;

import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// PathOps node handles path parsing and manipulation
export struct PathOps : arc::NodeImpl<trait::PathOps>
{
    auto normalise(std::string_view path) const -> std::string;
    auto parent(std::string_view path) const -> std::string;
    auto filename(std::string_view path) const -> std::string;
    auto join(std::string_view base, std::string_view child) const -> std::string;
    auto isRoot(std::string_view path) const -> bool;
};

} // namespace examples::filesystem::node
