export module arc.bench.compile99.node29;

import arc;
export import arc.bench.compile99.trait.trait28;
export import arc.bench.compile99.trait.trait29;

namespace arc::bench::compile99 {

export
struct Node29
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait28>;
        using Traits  = arc::Traits<Node, trait::Trait29>;

        int impl(trait::Trait29::get) const;

        Node() = default;
        int i = 29;
    };
};

}
