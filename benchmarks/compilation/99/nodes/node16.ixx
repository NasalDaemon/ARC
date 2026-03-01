export module arc.bench.compile99.node16;

import arc;
export import arc.bench.compile99.trait.trait15;
export import arc.bench.compile99.trait.trait16;

namespace arc::bench::compile99 {

export
struct Node16
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait15>;
        using Traits  = arc::Traits<Trait16>;

        int impl(Trait16::get) const;

        Node() = default;
        int i = 16;
    };
};

}
