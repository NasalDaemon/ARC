export module arc.bench.compile99.node14;

import arc;
export import arc.bench.compile99.trait.trait13;
export import arc.bench.compile99.trait.trait14;

namespace arc::bench::compile99 {

export
struct Node14
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait13>;
        using Traits  = arc::Traits<Trait14>;

        int impl(Trait14::get) const;

        Node() = default;
        int i = 14;
    };
};

}
