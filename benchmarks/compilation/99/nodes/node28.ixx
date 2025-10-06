export module arc.bench.compile99.node28;

import arc;
export import arc.bench.compile99.trait.trait27;
export import arc.bench.compile99.trait.trait28;

namespace arc::bench::compile99 {

export
struct Node28
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait27>;
        using Traits  = arc::Traits<Node, trait::Trait28>;

        int impl(trait::Trait28::get) const;

        Node() = default;
        int i = 28;
    };
};

}
