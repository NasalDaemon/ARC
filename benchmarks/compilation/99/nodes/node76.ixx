export module arc.bench.compile99.node76;

import arc;
export import arc.bench.compile99.trait.trait75;
export import arc.bench.compile99.trait.trait76;

namespace arc::bench::compile99 {

export
struct Node76
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait75>;
        using Traits  = arc::Traits<Trait76>;

        int impl(Trait76::get) const;

        Node() = default;
        int i = 76;
    };
};

}
