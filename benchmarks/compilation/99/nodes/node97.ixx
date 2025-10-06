export module arc.bench.compile99.node97;

import arc;
export import arc.bench.compile99.trait.trait96;
export import arc.bench.compile99.trait.trait97;

namespace arc::bench::compile99 {

export
struct Node97
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait96>;
        using Traits  = arc::Traits<Node, trait::Trait97>;

        int impl(trait::Trait97::get) const;

        Node() = default;
        int i = 97;
    };
};

}
