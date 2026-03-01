export module arc.bench.compile99.node83;

import arc;
export import arc.bench.compile99.trait.trait82;
export import arc.bench.compile99.trait.trait83;

namespace arc::bench::compile99 {

export
struct Node83
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait82>;
        using Traits  = arc::Traits<Trait83>;

        int impl(Trait83::get) const;

        Node() = default;
        int i = 83;
    };
};

}
