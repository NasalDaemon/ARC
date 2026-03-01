export module arc.bench.compile99.node15;

import arc;
export import arc.bench.compile99.trait.trait14;
export import arc.bench.compile99.trait.trait15;

namespace arc::bench::compile99 {

export
struct Node15
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait14>;
        using Traits  = arc::Traits<Trait15>;

        int impl(Trait15::get) const;

        Node() = default;
        int i = 15;
    };
};

}
