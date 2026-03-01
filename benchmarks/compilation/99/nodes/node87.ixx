export module arc.bench.compile99.node87;

import arc;
export import arc.bench.compile99.trait.trait86;
export import arc.bench.compile99.trait.trait87;

namespace arc::bench::compile99 {

export
struct Node87
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait86>;
        using Traits  = arc::Traits<Trait87>;

        int impl(Trait87::get) const;

        Node() = default;
        int i = 87;
    };
};

}
