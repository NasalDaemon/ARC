export module arc.bench.compile99.node69;

import arc;
export import arc.bench.compile99.trait.trait68;
export import arc.bench.compile99.trait.trait69;

namespace arc::bench::compile99 {

export
struct Node69
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait68>;
        using Traits  = arc::Traits<Node, trait::Trait69>;

        int impl(trait::Trait69::get) const;

        Node() = default;
        int i = 69;
    };
};

}
