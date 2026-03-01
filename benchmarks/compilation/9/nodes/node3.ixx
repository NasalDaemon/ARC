export module arc.bench.compile9.node3;

import arc;
export import arc.bench.compile9.trait.trait2;
export import arc.bench.compile9.trait.trait3;

namespace arc::bench::compile9 {

export
struct Node3
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait2>;
        using Traits  = arc::Traits<Trait3>;

        int impl(Trait3::get) const;

        Node() = default;
        int i = 3;
    };
};

}
