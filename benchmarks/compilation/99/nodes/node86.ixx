export module arc.bench.compile99.node86;

import arc;
export import arc.bench.compile99.trait.trait85;
export import arc.bench.compile99.trait.trait86;

namespace arc::bench::compile99 {

export
struct Node86
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait85>;
        using Traits  = arc::Traits<Trait86>;

        int impl(Trait86::get) const;

        Node() = default;
        int i = 86;
    };
};

}
