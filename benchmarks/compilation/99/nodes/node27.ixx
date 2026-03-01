export module arc.bench.compile99.node27;

import arc;
export import arc.bench.compile99.trait.trait26;
export import arc.bench.compile99.trait.trait27;

namespace arc::bench::compile99 {

export
struct Node27
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait26>;
        using Traits  = arc::Traits<Trait27>;

        int impl(Trait27::get) const;

        Node() = default;
        int i = 27;
    };
};

}
