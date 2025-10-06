export module arc.bench.compile99.node57;

import arc;
export import arc.bench.compile99.trait.trait56;
export import arc.bench.compile99.trait.trait57;

namespace arc::bench::compile99 {

export
struct Node57
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait56>;
        using Traits  = arc::Traits<Node, trait::Trait57>;

        int impl(trait::Trait57::get) const;

        Node() = default;
        int i = 57;
    };
};

}
