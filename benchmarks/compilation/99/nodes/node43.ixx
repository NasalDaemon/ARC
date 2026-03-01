export module arc.bench.compile99.node43;

import arc;
export import arc.bench.compile99.trait.trait42;
export import arc.bench.compile99.trait.trait43;

namespace arc::bench::compile99 {

export
struct Node43
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait42>;
        using Traits  = arc::Traits<Trait43>;

        int impl(Trait43::get) const;

        Node() = default;
        int i = 43;
    };
};

}
