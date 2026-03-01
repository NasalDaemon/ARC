export module arc.bench.compile99.node53;

import arc;
export import arc.bench.compile99.trait.trait52;
export import arc.bench.compile99.trait.trait53;

namespace arc::bench::compile99 {

export
struct Node53
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait52>;
        using Traits  = arc::Traits<Trait53>;

        int impl(Trait53::get) const;

        Node() = default;
        int i = 53;
    };
};

}
