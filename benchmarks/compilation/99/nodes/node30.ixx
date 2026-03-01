export module arc.bench.compile99.node30;

import arc;
export import arc.bench.compile99.trait.trait29;
export import arc.bench.compile99.trait.trait30;

namespace arc::bench::compile99 {

export
struct Node30
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait29>;
        using Traits  = arc::Traits<Trait30>;

        int impl(Trait30::get) const;

        Node() = default;
        int i = 30;
    };
};

}
