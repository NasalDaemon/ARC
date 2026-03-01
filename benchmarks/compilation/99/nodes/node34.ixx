export module arc.bench.compile99.node34;

import arc;
export import arc.bench.compile99.trait.trait33;
export import arc.bench.compile99.trait.trait34;

namespace arc::bench::compile99 {

export
struct Node34
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait33>;
        using Traits  = arc::Traits<Trait34>;

        int impl(Trait34::get) const;

        Node() = default;
        int i = 34;
    };
};

}
