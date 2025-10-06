export module arc.bench.compile99.node58;

import arc;
export import arc.bench.compile99.trait.trait57;
export import arc.bench.compile99.trait.trait58;

namespace arc::bench::compile99 {

export
struct Node58
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait57>;
        using Traits  = arc::Traits<Node, trait::Trait58>;

        int impl(trait::Trait58::get) const;

        Node() = default;
        int i = 58;
    };
};

}
