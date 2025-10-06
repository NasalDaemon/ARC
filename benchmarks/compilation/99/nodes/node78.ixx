export module arc.bench.compile99.node78;

import arc;
export import arc.bench.compile99.trait.trait77;
export import arc.bench.compile99.trait.trait78;

namespace arc::bench::compile99 {

export
struct Node78
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait77>;
        using Traits  = arc::Traits<Node, trait::Trait78>;

        int impl(trait::Trait78::get) const;

        Node() = default;
        int i = 78;
    };
};

}
