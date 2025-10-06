export module arc.bench.compile99.node94;

import arc;
export import arc.bench.compile99.trait.trait93;
export import arc.bench.compile99.trait.trait94;

namespace arc::bench::compile99 {

export
struct Node94
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait93>;
        using Traits  = arc::Traits<Node, trait::Trait94>;

        int impl(trait::Trait94::get) const;

        Node() = default;
        int i = 94;
    };
};

}
