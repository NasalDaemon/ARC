export module arc.bench.compile99.node64;

import arc;
export import arc.bench.compile99.trait.trait63;
export import arc.bench.compile99.trait.trait64;

namespace arc::bench::compile99 {

export
struct Node64
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait63>;
        using Traits  = arc::Traits<Node, trait::Trait64>;

        int impl(trait::Trait64::get) const;

        Node() = default;
        int i = 64;
    };
};

}
