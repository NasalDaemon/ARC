export module examples.calculator.file_persistence;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct FilePersistence
{
    template<class Context>
    struct Node : arc::Node::
        Impl<Persistence>::
        Uses<Variables>
    {
        auto save(std::string_view path) -> std::expected<void, std::string>;
        auto load(std::string_view path) -> std::expected<void, std::string>;
    };
};

}
