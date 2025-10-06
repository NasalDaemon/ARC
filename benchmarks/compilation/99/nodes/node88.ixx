export module arc.bench.compile99.node88;

import arc;
export import arc.bench.compile99.trait.trait87;
export import arc.bench.compile99.trait.trait88;

namespace arc::bench::compile99 {

export
struct Node88
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait87>;
        using Traits  = arc::Traits<Node, trait::Trait88>;

        int impl(trait::Trait88::get) const;

        Node() = default;
        int i = 88;
    };
};

}
