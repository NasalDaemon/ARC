export module arc.bench.compile99.node89;

import arc;
export import arc.bench.compile99.trait.trait88;
export import arc.bench.compile99.trait.trait89;

namespace arc::bench::compile99 {

export
struct Node89
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait88>;
        using Traits  = arc::Traits<Trait89>;

        int impl(Trait89::get) const;

        Node() = default;
        int i = 89;
    };
};

}
