export module arc.bench.compile99.node50;

import arc;
export import arc.bench.compile99.trait.trait49;
export import arc.bench.compile99.trait.trait50;

namespace arc::bench::compile99 {

export
struct Node50
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait49>;
        using Traits  = arc::Traits<Node, trait::Trait50>;

        int impl(trait::Trait50::get) const;

        Node() = default;
        int i = 50;
    };
};

}
