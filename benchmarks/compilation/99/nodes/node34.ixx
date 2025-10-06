export module arc.bench.compile99.node34;

import arc;
export import arc.bench.compile99.trait.trait33;
export import arc.bench.compile99.trait.trait34;

namespace arc::bench::compile99 {

export
struct Node34
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait33>;
        using Traits  = arc::Traits<Node, trait::Trait34>;

        int impl(trait::Trait34::get) const;

        Node() = default;
        int i = 34;
    };
};

}
