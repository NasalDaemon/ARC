export module arc.bench.compile9.node8;

import arc;
export import arc.bench.compile9.trait.trait7;
export import arc.bench.compile9.trait.trait8;

namespace arc::bench::compile9 {

export
struct Node8
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait7>;
        using Traits  = arc::Traits<Node, trait::Trait8>;

        int impl(trait::Trait8::get) const;

        Node() = default;
        int i = 8;
    };
};

}
