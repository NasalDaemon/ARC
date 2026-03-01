export module arc.bench.compile99.node98;

import arc;
export import arc.bench.compile99.trait.trait97;
export import arc.bench.compile99.trait.trait98;

namespace arc::bench::compile99 {

export
struct Node98
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait97>;
        using Traits  = arc::Traits<Trait98>;

        int impl(Trait98::get) const;

        Node() = default;
        int i = 98;
    };
};

}
