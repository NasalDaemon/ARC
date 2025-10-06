export module arc.bench.compile99.node72;

import arc;
export import arc.bench.compile99.trait.trait71;
export import arc.bench.compile99.trait.trait72;

namespace arc::bench::compile99 {

export
struct Node72
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait71>;
        using Traits  = arc::Traits<Node, trait::Trait72>;

        int impl(trait::Trait72::get) const;

        Node() = default;
        int i = 72;
    };
};

}
