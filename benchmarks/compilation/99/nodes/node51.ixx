export module arc.bench.compile99.node51;

import arc;
export import arc.bench.compile99.trait.trait50;
export import arc.bench.compile99.trait.trait51;

namespace arc::bench::compile99 {

export
struct Node51
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait50>;
        using Traits  = arc::Traits<Trait51>;

        int impl(Trait51::get) const;

        Node() = default;
        int i = 51;
    };
};

}
