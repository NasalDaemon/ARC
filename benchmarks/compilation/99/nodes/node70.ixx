export module arc.bench.compile99.node70;

import arc;
export import arc.bench.compile99.trait.trait69;
export import arc.bench.compile99.trait.trait70;

namespace arc::bench::compile99 {

export
struct Node70
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait69>;
        using Traits  = arc::Traits<Trait70>;

        int impl(Trait70::get) const;

        Node() = default;
        int i = 70;
    };
};

}
