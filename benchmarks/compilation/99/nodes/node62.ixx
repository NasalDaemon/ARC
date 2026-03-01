export module arc.bench.compile99.node62;

import arc;
export import arc.bench.compile99.trait.trait61;
export import arc.bench.compile99.trait.trait62;

namespace arc::bench::compile99 {

export
struct Node62
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait61>;
        using Traits  = arc::Traits<Trait62>;

        int impl(Trait62::get) const;

        Node() = default;
        int i = 62;
    };
};

}
