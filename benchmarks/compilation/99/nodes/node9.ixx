export module arc.bench.compile99.node9;

import arc;
export import arc.bench.compile99.trait.trait8;
export import arc.bench.compile99.trait.trait9;

namespace arc::bench::compile99 {

export
struct Node9
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait8>;
        using Traits  = arc::Traits<Node, trait::Trait9>;

        int impl(trait::Trait9::get) const;

        Node() = default;
        int i = 9;
    };
};

}
