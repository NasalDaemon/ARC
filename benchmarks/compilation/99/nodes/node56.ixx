export module arc.bench.compile99.node56;

import arc;
export import arc.bench.compile99.trait.trait55;
export import arc.bench.compile99.trait.trait56;

namespace arc::bench::compile99 {

export
struct Node56
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait55>;
        using Traits  = arc::Traits<Node, trait::Trait56>;

        int impl(trait::Trait56::get) const;

        Node() = default;
        int i = 56;
    };
};

}
