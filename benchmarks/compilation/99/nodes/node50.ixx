export module arc.bench.compile99.node50;

import arc;
export import arc.bench.compile99.trait.trait49;
export import arc.bench.compile99.trait.trait50;

namespace arc::bench::compile99 {

export
struct Node50
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait49>;
        using Traits  = arc::Traits<Trait50>;

        int impl(Trait50::get) const;

        Node() = default;
        int i = 50;
    };
};

}
