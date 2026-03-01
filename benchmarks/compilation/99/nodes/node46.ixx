export module arc.bench.compile99.node46;

import arc;
export import arc.bench.compile99.trait.trait45;
export import arc.bench.compile99.trait.trait46;

namespace arc::bench::compile99 {

export
struct Node46
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait45>;
        using Traits  = arc::Traits<Trait46>;

        int impl(Trait46::get) const;

        Node() = default;
        int i = 46;
    };
};

}
