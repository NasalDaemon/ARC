export module arc.bench.compile99.node31;

import arc;
export import arc.bench.compile99.trait.trait30;
export import arc.bench.compile99.trait.trait31;

namespace arc::bench::compile99 {

export
struct Node31
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait30>;
        using Traits  = arc::Traits<Node, trait::Trait31>;

        int impl(trait::Trait31::get) const;

        Node() = default;
        int i = 31;
    };
};

}
