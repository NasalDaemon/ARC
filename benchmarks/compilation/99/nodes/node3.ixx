export module arc.bench.compile99.node3;

import arc;
export import arc.bench.compile99.trait.trait2;
export import arc.bench.compile99.trait.trait3;

namespace arc::bench::compile99 {

export
struct Node3
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait2>;
        using Traits  = arc::Traits<Node, trait::Trait3>;

        int impl(trait::Trait3::get) const;

        Node() = default;
        int i = 3;
    };
};

}
