export module arc.bench.compile99.node66;

import arc;
export import arc.bench.compile99.trait.trait65;
export import arc.bench.compile99.trait.trait66;

namespace arc::bench::compile99 {

export
struct Node66
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait65>;
        using Traits  = arc::Traits<Node, trait::Trait66>;

        int impl(trait::Trait66::get) const;

        Node() = default;
        int i = 66;
    };
};

}
