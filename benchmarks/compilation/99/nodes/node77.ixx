export module arc.bench.compile99.node77;

import arc;
export import arc.bench.compile99.trait.trait76;
export import arc.bench.compile99.trait.trait77;

namespace arc::bench::compile99 {

export
struct Node77
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait76>;
        using Traits  = arc::Traits<Trait77>;

        int impl(Trait77::get) const;

        Node() = default;
        int i = 77;
    };
};

}
