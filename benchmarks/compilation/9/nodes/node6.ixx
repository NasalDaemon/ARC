export module arc.bench.compile9.node6;

import arc;
export import arc.bench.compile9.trait.trait5;
export import arc.bench.compile9.trait.trait6;

namespace arc::bench::compile9 {

export
struct Node6
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait5>;
        using Traits  = arc::Traits<Node, trait::Trait6>;

        int impl(trait::Trait6::get) const;

        Node() = default;
        int i = 6;
    };
};

}
