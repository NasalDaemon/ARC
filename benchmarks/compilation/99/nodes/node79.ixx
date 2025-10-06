export module arc.bench.compile99.node79;

import arc;
export import arc.bench.compile99.trait.trait78;
export import arc.bench.compile99.trait.trait79;

namespace arc::bench::compile99 {

export
struct Node79
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait78>;
        using Traits  = arc::Traits<Node, trait::Trait79>;

        int impl(trait::Trait79::get) const;

        Node() = default;
        int i = 79;
    };
};

}
