export module arc.bench.compile99.node30;

import arc;
export import arc.bench.compile99.trait.trait29;
export import arc.bench.compile99.trait.trait30;

namespace arc::bench::compile99 {

export
struct Node30
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait29>;
        using Traits  = arc::Traits<Node, trait::Trait30>;

        int impl(trait::Trait30::get) const;

        Node() = default;
        int i = 30;
    };
};

}
