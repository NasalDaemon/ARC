export module arc.bench.compile99.node54;

import arc;
export import arc.bench.compile99.trait.trait53;
export import arc.bench.compile99.trait.trait54;

namespace arc::bench::compile99 {

export
struct Node54
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait53>;
        using Traits  = arc::Traits<Trait54>;

        int impl(Trait54::get) const;

        Node() = default;
        int i = 54;
    };
};

}
