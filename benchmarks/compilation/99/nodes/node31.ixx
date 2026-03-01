export module arc.bench.compile99.node31;

import arc;
export import arc.bench.compile99.trait.trait30;
export import arc.bench.compile99.trait.trait31;

namespace arc::bench::compile99 {

export
struct Node31
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait30>;
        using Traits  = arc::Traits<Trait31>;

        int impl(Trait31::get) const;

        Node() = default;
        int i = 31;
    };
};

}
