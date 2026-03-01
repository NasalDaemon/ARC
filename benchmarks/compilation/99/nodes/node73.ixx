export module arc.bench.compile99.node73;

import arc;
export import arc.bench.compile99.trait.trait72;
export import arc.bench.compile99.trait.trait73;

namespace arc::bench::compile99 {

export
struct Node73
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait72>;
        using Traits  = arc::Traits<Trait73>;

        int impl(Trait73::get) const;

        Node() = default;
        int i = 73;
    };
};

}
