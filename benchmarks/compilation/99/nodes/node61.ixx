export module arc.bench.compile99.node61;

import arc;
export import arc.bench.compile99.trait.trait60;
export import arc.bench.compile99.trait.trait61;

namespace arc::bench::compile99 {

export
struct Node61
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait60>;
        using Traits  = arc::Traits<Node, trait::Trait61>;

        int impl(trait::Trait61::get) const;

        Node() = default;
        int i = 61;
    };
};

}
