export module arc.bench.compile99.node99;

import arc;
export import arc.bench.compile99.trait.trait98;
export import arc.bench.compile99.trait.trait99;

namespace arc::bench::compile99 {

export
struct Node99
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait98>;
        using Traits  = arc::Traits<Node, trait::Trait99>;

        int impl(trait::Trait99::get) const;

        Node() = default;
        int i = 99;
    };
};

}
