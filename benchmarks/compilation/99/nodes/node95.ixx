export module arc.bench.compile99.node95;

import arc;
export import arc.bench.compile99.trait.trait94;
export import arc.bench.compile99.trait.trait95;

namespace arc::bench::compile99 {

export
struct Node95
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait94>;
        using Traits  = arc::Traits<Node, trait::Trait95>;

        int impl(trait::Trait95::get) const;

        Node() = default;
        int i = 95;
    };
};

}
