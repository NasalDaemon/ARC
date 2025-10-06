export module arc.bench.compile99.node51;

import arc;
export import arc.bench.compile99.trait.trait50;
export import arc.bench.compile99.trait.trait51;

namespace arc::bench::compile99 {

export
struct Node51
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait50>;
        using Traits  = arc::Traits<Node, trait::Trait51>;

        int impl(trait::Trait51::get) const;

        Node() = default;
        int i = 51;
    };
};

}
