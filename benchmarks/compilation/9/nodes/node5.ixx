export module arc.bench.compile9.node5;

import arc;
export import arc.bench.compile9.trait.trait4;
export import arc.bench.compile9.trait.trait5;

namespace arc::bench::compile9 {

export
struct Node5
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait4>;
        using Traits  = arc::Traits<Node, trait::Trait5>;

        int impl(trait::Trait5::get) const;

        Node() = default;
        int i = 5;
    };
};

}
