export module arc.bench.compile99.node52;

import arc;
export import arc.bench.compile99.trait.trait51;
export import arc.bench.compile99.trait.trait52;

namespace arc::bench::compile99 {

export
struct Node52
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait51>;
        using Traits  = arc::Traits<Trait52>;

        int impl(Trait52::get) const;

        Node() = default;
        int i = 52;
    };
};

}
