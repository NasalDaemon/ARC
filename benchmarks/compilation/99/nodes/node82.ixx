export module arc.bench.compile99.node82;

import arc;
export import arc.bench.compile99.trait.trait81;
export import arc.bench.compile99.trait.trait82;

namespace arc::bench::compile99 {

export
struct Node82
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait81>;
        using Traits  = arc::Traits<Node, trait::Trait82>;

        int impl(trait::Trait82::get) const;

        Node() = default;
        int i = 82;
    };
};

}
