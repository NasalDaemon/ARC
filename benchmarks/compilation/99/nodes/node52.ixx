export module arc.bench.compile99.node52;

import arc;
export import arc.bench.compile99.trait.trait51;
export import arc.bench.compile99.trait.trait52;

namespace arc::bench::compile99 {

export
struct Node52
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait51>;
        using Traits  = arc::Traits<Node, trait::Trait52>;

        int impl(trait::Trait52::get) const;

        Node() = default;
        int i = 52;
    };
};

}
