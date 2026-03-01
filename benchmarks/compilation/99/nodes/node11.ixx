export module arc.bench.compile99.node11;

import arc;
export import arc.bench.compile99.trait.trait10;
export import arc.bench.compile99.trait.trait11;

namespace arc::bench::compile99 {

export
struct Node11
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait10>;
        using Traits  = arc::Traits<Trait11>;

        int impl(Trait11::get) const;

        Node() = default;
        int i = 11;
    };
};

}
