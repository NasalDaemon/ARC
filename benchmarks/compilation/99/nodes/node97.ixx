export module arc.bench.compile99.node97;

import arc;
export import arc.bench.compile99.trait.trait96;
export import arc.bench.compile99.trait.trait97;

namespace arc::bench::compile99 {

export
struct Node97
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait96>;
        using Traits  = arc::Traits<Trait97>;

        int impl(Trait97::get) const;

        Node() = default;
        int i = 97;
    };
};

}
