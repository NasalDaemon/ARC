export module arc.bench.compile99.node96;

import arc;
export import arc.bench.compile99.trait.trait95;
export import arc.bench.compile99.trait.trait96;

namespace arc::bench::compile99 {

export
struct Node96
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait95>;
        using Traits  = arc::Traits<Node, trait::Trait96>;

        int impl(trait::Trait96::get) const;

        Node() = default;
        int i = 96;
    };
};

}
