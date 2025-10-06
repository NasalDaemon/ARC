export module arc.bench.compile99.node23;

import arc;
export import arc.bench.compile99.trait.trait22;
export import arc.bench.compile99.trait.trait23;

namespace arc::bench::compile99 {

export
struct Node23
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait22>;
        using Traits  = arc::Traits<Node, trait::Trait23>;

        int impl(trait::Trait23::get) const;

        Node() = default;
        int i = 23;
    };
};

}
