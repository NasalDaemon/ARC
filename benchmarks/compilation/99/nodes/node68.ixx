export module arc.bench.compile99.node68;

import arc;
export import arc.bench.compile99.trait.trait67;
export import arc.bench.compile99.trait.trait68;

namespace arc::bench::compile99 {

export
struct Node68
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait67>;
        using Traits  = arc::Traits<Node, trait::Trait68>;

        int impl(trait::Trait68::get) const;

        Node() = default;
        int i = 68;
    };
};

}
