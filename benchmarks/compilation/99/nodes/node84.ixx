export module arc.bench.compile99.node84;

import arc;
export import arc.bench.compile99.trait.trait83;
export import arc.bench.compile99.trait.trait84;

namespace arc::bench::compile99 {

export
struct Node84
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait83>;
        using Traits  = arc::Traits<Node, trait::Trait84>;

        int impl(trait::Trait84::get) const;

        Node() = default;
        int i = 84;
    };
};

}
