export module arc.bench.compile99.node41;

import arc;
export import arc.bench.compile99.trait.trait40;
export import arc.bench.compile99.trait.trait41;

namespace arc::bench::compile99 {

export
struct Node41
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait40>;
        using Traits  = arc::Traits<Node, trait::Trait41>;

        int impl(trait::Trait41::get) const;

        Node() = default;
        int i = 41;
    };
};

}
