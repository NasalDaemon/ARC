export module arc.bench.compile99.node4;

import arc;
export import arc.bench.compile99.trait.trait3;
export import arc.bench.compile99.trait.trait4;

namespace arc::bench::compile99 {

export
struct Node4
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait3>;
        using Traits  = arc::Traits<Trait4>;

        int impl(Trait4::get) const;

        Node() = default;
        int i = 4;
    };
};

}
