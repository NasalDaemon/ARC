export module arc.bench.compile99.node33;

import arc;
export import arc.bench.compile99.trait.trait32;
export import arc.bench.compile99.trait.trait33;

namespace arc::bench::compile99 {

export
struct Node33
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait32>;
        using Traits  = arc::Traits<Node, trait::Trait33>;

        int impl(trait::Trait33::get) const;

        Node() = default;
        int i = 33;
    };
};

}
