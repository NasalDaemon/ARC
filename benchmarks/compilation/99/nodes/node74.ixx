export module arc.bench.compile99.node74;

import arc;
export import arc.bench.compile99.trait.trait73;
export import arc.bench.compile99.trait.trait74;

namespace arc::bench::compile99 {

export
struct Node74
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait73>;
        using Traits  = arc::Traits<Node, trait::Trait74>;

        int impl(trait::Trait74::get) const;

        Node() = default;
        int i = 74;
    };
};

}
