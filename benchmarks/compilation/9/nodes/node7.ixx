export module arc.bench.compile9.node7;

import arc;
export import arc.bench.compile9.trait.trait6;
export import arc.bench.compile9.trait.trait7;

namespace arc::bench::compile9 {

export
struct Node7
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait6>;
        using Traits  = arc::Traits<Trait7>;

        int impl(Trait7::get) const;

        Node() = default;
        int i = 7;
    };
};

}
