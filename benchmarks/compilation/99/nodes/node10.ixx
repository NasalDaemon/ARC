export module arc.bench.compile99.node10;

import arc;
export import arc.bench.compile99.trait.trait9;
export import arc.bench.compile99.trait.trait10;

namespace arc::bench::compile99 {

export
struct Node10
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait9>;
        using Traits  = arc::Traits<Trait10>;

        int impl(Trait10::get) const;

        Node() = default;
        int i = 10;
    };
};

}
