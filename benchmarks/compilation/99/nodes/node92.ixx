export module arc.bench.compile99.node92;

import arc;
export import arc.bench.compile99.trait.trait91;
export import arc.bench.compile99.trait.trait92;

namespace arc::bench::compile99 {

export
struct Node92
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait91>;
        using Traits  = arc::Traits<Node, trait::Trait92>;

        int impl(trait::Trait92::get) const;

        Node() = default;
        int i = 92;
    };
};

}
