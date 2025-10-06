export module arc.bench.compile99.node63;

import arc;
export import arc.bench.compile99.trait.trait62;
export import arc.bench.compile99.trait.trait63;

namespace arc::bench::compile99 {

export
struct Node63
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait62>;
        using Traits  = arc::Traits<Node, trait::Trait63>;

        int impl(trait::Trait63::get) const;

        Node() = default;
        int i = 63;
    };
};

}
