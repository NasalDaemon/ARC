export module arc.bench.compile99.node18;

import arc;
export import arc.bench.compile99.trait.trait17;
export import arc.bench.compile99.trait.trait18;

namespace arc::bench::compile99 {

export
struct Node18
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait17>;
        using Traits  = arc::Traits<Trait18>;

        int impl(Trait18::get) const;

        Node() = default;
        int i = 18;
    };
};

}
