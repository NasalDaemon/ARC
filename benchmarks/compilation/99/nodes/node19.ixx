export module arc.bench.compile99.node19;

import arc;
export import arc.bench.compile99.trait.trait18;
export import arc.bench.compile99.trait.trait19;

namespace arc::bench::compile99 {

export
struct Node19
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait18>;
        using Traits  = arc::Traits<Trait19>;

        int impl(Trait19::get) const;

        Node() = default;
        int i = 19;
    };
};

}
