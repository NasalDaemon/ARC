export module arc.bench.compile99.node90;

import arc;
export import arc.bench.compile99.trait.trait89;
export import arc.bench.compile99.trait.trait90;

namespace arc::bench::compile99 {

export
struct Node90
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait89>;
        using Traits  = arc::Traits<Node, trait::Trait90>;

        int impl(trait::Trait90::get) const;

        Node() = default;
        int i = 90;
    };
};

}
