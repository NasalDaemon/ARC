export module arc.bench.compile99.node20;

import arc;
export import arc.bench.compile99.trait.trait19;
export import arc.bench.compile99.trait.trait20;

namespace arc::bench::compile99 {

export
struct Node20
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait19>;
        using Traits  = arc::Traits<Node, trait::Trait20>;

        int impl(trait::Trait20::get) const;

        Node() = default;
        int i = 20;
    };
};

}
