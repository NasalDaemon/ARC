export module arc.bench.compile99.node67;

import arc;
export import arc.bench.compile99.trait.trait66;
export import arc.bench.compile99.trait.trait67;

namespace arc::bench::compile99 {

export
struct Node67
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait66>;
        using Traits  = arc::Traits<Node, trait::Trait67>;

        int impl(trait::Trait67::get) const;

        Node() = default;
        int i = 67;
    };
};

}
