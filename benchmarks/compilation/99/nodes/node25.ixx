export module arc.bench.compile99.node25;

import arc;
export import arc.bench.compile99.trait.trait24;
export import arc.bench.compile99.trait.trait25;

namespace arc::bench::compile99 {

export
struct Node25
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait24>;
        using Traits  = arc::Traits<Node, trait::Trait25>;

        int impl(trait::Trait25::get) const;

        Node() = default;
        int i = 25;
    };
};

}
