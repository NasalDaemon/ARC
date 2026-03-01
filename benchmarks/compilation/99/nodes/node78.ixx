export module arc.bench.compile99.node78;

import arc;
export import arc.bench.compile99.trait.trait77;
export import arc.bench.compile99.trait.trait78;

namespace arc::bench::compile99 {

export
struct Node78
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait77>;
        using Traits  = arc::Traits<Trait78>;

        int impl(Trait78::get) const;

        Node() = default;
        int i = 78;
    };
};

}
