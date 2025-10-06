export module arc.bench.compile99.node22;

import arc;
export import arc.bench.compile99.trait.trait21;
export import arc.bench.compile99.trait.trait22;

namespace arc::bench::compile99 {

export
struct Node22
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait21>;
        using Traits  = arc::Traits<Node, trait::Trait22>;

        int impl(trait::Trait22::get) const;

        Node() = default;
        int i = 22;
    };
};

}
