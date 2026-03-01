export module arc.bench.compile99.node75;

import arc;
export import arc.bench.compile99.trait.trait74;
export import arc.bench.compile99.trait.trait75;

namespace arc::bench::compile99 {

export
struct Node75
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait74>;
        using Traits  = arc::Traits<Trait75>;

        int impl(Trait75::get) const;

        Node() = default;
        int i = 75;
    };
};

}
