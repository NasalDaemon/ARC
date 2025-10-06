export module arc.bench.compile99.node91;

import arc;
export import arc.bench.compile99.trait.trait90;
export import arc.bench.compile99.trait.trait91;

namespace arc::bench::compile99 {

export
struct Node91
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait90>;
        using Traits  = arc::Traits<Node, trait::Trait91>;

        int impl(trait::Trait91::get) const;

        Node() = default;
        int i = 91;
    };
};

}
