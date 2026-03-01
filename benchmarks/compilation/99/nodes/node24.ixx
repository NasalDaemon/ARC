export module arc.bench.compile99.node24;

import arc;
export import arc.bench.compile99.trait.trait23;
export import arc.bench.compile99.trait.trait24;

namespace arc::bench::compile99 {

export
struct Node24
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait23>;
        using Traits  = arc::Traits<Trait24>;

        int impl(Trait24::get) const;

        Node() = default;
        int i = 24;
    };
};

}
