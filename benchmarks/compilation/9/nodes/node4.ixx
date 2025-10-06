export module arc.bench.compile9.node4;

import arc;
export import arc.bench.compile9.trait.trait3;
export import arc.bench.compile9.trait.trait4;

namespace arc::bench::compile9 {

export
struct Node4
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait3>;
        using Traits  = arc::Traits<Node, trait::Trait4>;

        int impl(trait::Trait4::get) const;

        Node() = default;
        int i = 4;
    };
};

}
