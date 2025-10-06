export module arc.bench.compile99.node37;

import arc;
export import arc.bench.compile99.trait.trait36;
export import arc.bench.compile99.trait.trait37;

namespace arc::bench::compile99 {

export
struct Node37
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait36>;
        using Traits  = arc::Traits<Node, trait::Trait37>;

        int impl(trait::Trait37::get) const;

        Node() = default;
        int i = 37;
    };
};

}
