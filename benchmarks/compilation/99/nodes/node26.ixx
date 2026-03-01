export module arc.bench.compile99.node26;

import arc;
export import arc.bench.compile99.trait.trait25;
export import arc.bench.compile99.trait.trait26;

namespace arc::bench::compile99 {

export
struct Node26
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait25>;
        using Traits  = arc::Traits<Trait26>;

        int impl(Trait26::get) const;

        Node() = default;
        int i = 26;
    };
};

}
