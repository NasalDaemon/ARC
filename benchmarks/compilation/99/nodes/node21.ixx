export module arc.bench.compile99.node21;

import arc;
export import arc.bench.compile99.trait.trait20;
export import arc.bench.compile99.trait.trait21;

namespace arc::bench::compile99 {

export
struct Node21
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait20>;
        using Traits  = arc::Traits<Trait21>;

        int impl(Trait21::get) const;

        Node() = default;
        int i = 21;
    };
};

}
