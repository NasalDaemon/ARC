export module arc.bench.compile99.node22;

import arc;
export import arc.bench.compile99.trait.trait21;
export import arc.bench.compile99.trait.trait22;

namespace arc::bench::compile99 {

export
struct Node22
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait21>;
        using Traits  = arc::Traits<Trait22>;

        int impl(Trait22::get) const;

        Node() = default;
        int i = 22;
    };
};

}
