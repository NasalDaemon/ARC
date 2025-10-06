export module arc.bench.compile99.node16;

import arc;
export import arc.bench.compile99.trait.trait15;
export import arc.bench.compile99.trait.trait16;

namespace arc::bench::compile99 {

export
struct Node16
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait15>;
        using Traits  = arc::Traits<Node, trait::Trait16>;

        int impl(trait::Trait16::get) const;

        Node() = default;
        int i = 16;
    };
};

}
