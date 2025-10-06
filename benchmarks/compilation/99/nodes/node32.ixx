export module arc.bench.compile99.node32;

import arc;
export import arc.bench.compile99.trait.trait31;
export import arc.bench.compile99.trait.trait32;

namespace arc::bench::compile99 {

export
struct Node32
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait31>;
        using Traits  = arc::Traits<Node, trait::Trait32>;

        int impl(trait::Trait32::get) const;

        Node() = default;
        int i = 32;
    };
};

}
