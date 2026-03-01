export module arc.bench.compile99.node67;

import arc;
export import arc.bench.compile99.trait.trait66;
export import arc.bench.compile99.trait.trait67;

namespace arc::bench::compile99 {

export
struct Node67
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait66>;
        using Traits  = arc::Traits<Trait67>;

        int impl(Trait67::get) const;

        Node() = default;
        int i = 67;
    };
};

}
