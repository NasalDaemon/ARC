export module arc.bench.compile99.node24;

import arc;
export import arc.bench.compile99.trait.trait23;
export import arc.bench.compile99.trait.trait24;

namespace arc::bench::compile99 {

export
struct Node24
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait23>;
        using Traits  = arc::Traits<Node, trait::Trait24>;

        int impl(trait::Trait24::get) const;

        Node() = default;
        int i = 24;
    };
};

}
