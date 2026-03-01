export module arc.bench.compile99.node81;

import arc;
export import arc.bench.compile99.trait.trait80;
export import arc.bench.compile99.trait.trait81;

namespace arc::bench::compile99 {

export
struct Node81
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait80>;
        using Traits  = arc::Traits<Trait81>;

        int impl(Trait81::get) const;

        Node() = default;
        int i = 81;
    };
};

}
