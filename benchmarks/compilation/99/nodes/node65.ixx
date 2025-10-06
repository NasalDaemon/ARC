export module arc.bench.compile99.node65;

import arc;
export import arc.bench.compile99.trait.trait64;
export import arc.bench.compile99.trait.trait65;

namespace arc::bench::compile99 {

export
struct Node65
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait64>;
        using Traits  = arc::Traits<Node, trait::Trait65>;

        int impl(trait::Trait65::get) const;

        Node() = default;
        int i = 65;
    };
};

}
