export module arc.bench.compile99.node93;

import arc;
export import arc.bench.compile99.trait.trait92;
export import arc.bench.compile99.trait.trait93;

namespace arc::bench::compile99 {

export
struct Node93
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait92>;
        using Traits  = arc::Traits<Node, trait::Trait93>;

        int impl(trait::Trait93::get) const;

        Node() = default;
        int i = 93;
    };
};

}
