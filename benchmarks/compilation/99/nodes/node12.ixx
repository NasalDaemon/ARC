export module arc.bench.compile99.node12;

import arc;
export import arc.bench.compile99.trait.trait11;
export import arc.bench.compile99.trait.trait12;

namespace arc::bench::compile99 {

export
struct Node12
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait11>;
        using Traits  = arc::Traits<Trait12>;

        int impl(Trait12::get) const;

        Node() = default;
        int i = 12;
    };
};

}
