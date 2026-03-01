export module arc.bench.compile99.node5;

import arc;
export import arc.bench.compile99.trait.trait4;
export import arc.bench.compile99.trait.trait5;

namespace arc::bench::compile99 {

export
struct Node5
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait4>;
        using Traits  = arc::Traits<Trait5>;

        int impl(Trait5::get) const;

        Node() = default;
        int i = 5;
    };
};

}
