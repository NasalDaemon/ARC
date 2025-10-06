export module arc.bench.compile99.node42;

import arc;
export import arc.bench.compile99.trait.trait41;
export import arc.bench.compile99.trait.trait42;

namespace arc::bench::compile99 {

export
struct Node42
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait41>;
        using Traits  = arc::Traits<Node, trait::Trait42>;

        int impl(trait::Trait42::get) const;

        Node() = default;
        int i = 42;
    };
};

}
