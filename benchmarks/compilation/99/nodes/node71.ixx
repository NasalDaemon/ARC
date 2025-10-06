export module arc.bench.compile99.node71;

import arc;
export import arc.bench.compile99.trait.trait70;
export import arc.bench.compile99.trait.trait71;

namespace arc::bench::compile99 {

export
struct Node71
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait70>;
        using Traits  = arc::Traits<Node, trait::Trait71>;

        int impl(trait::Trait71::get) const;

        Node() = default;
        int i = 71;
    };
};

}
