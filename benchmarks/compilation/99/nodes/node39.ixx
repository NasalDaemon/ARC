export module arc.bench.compile99.node39;

import arc;
export import arc.bench.compile99.trait.trait38;
export import arc.bench.compile99.trait.trait39;

namespace arc::bench::compile99 {

export
struct Node39
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait38>;
        using Traits  = arc::Traits<Node, trait::Trait39>;

        int impl(trait::Trait39::get) const;

        Node() = default;
        int i = 39;
    };
};

}
