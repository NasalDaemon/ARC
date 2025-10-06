export module arc.bench.compile99.node55;

import arc;
export import arc.bench.compile99.trait.trait54;
export import arc.bench.compile99.trait.trait55;

namespace arc::bench::compile99 {

export
struct Node55
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait54>;
        using Traits  = arc::Traits<Node, trait::Trait55>;

        int impl(trait::Trait55::get) const;

        Node() = default;
        int i = 55;
    };
};

}
