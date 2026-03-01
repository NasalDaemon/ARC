export module arc.bench.compile99.node28;

import arc;
export import arc.bench.compile99.trait.trait27;
export import arc.bench.compile99.trait.trait28;

namespace arc::bench::compile99 {

export
struct Node28
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait27>;
        using Traits  = arc::Traits<Trait28>;

        int impl(Trait28::get) const;

        Node() = default;
        int i = 28;
    };
};

}
