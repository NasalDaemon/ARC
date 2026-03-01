export module arc.bench.compile99.node2;

import arc;
export import arc.bench.compile99.trait.trait1;
export import arc.bench.compile99.trait.trait2;

namespace arc::bench::compile99 {

export
struct Node2
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait1>;
        using Traits = arc::Traits<Trait2>;

        int impl(Trait2::get) const;

        Node() = default;
        int i = 2;
    };
};

}
