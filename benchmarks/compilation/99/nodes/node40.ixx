export module arc.bench.compile99.node40;

import arc;
export import arc.bench.compile99.trait.trait39;
export import arc.bench.compile99.trait.trait40;

namespace arc::bench::compile99 {

export
struct Node40
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait39>;
        using Traits  = arc::Traits<Node, trait::Trait40>;

        int impl(trait::Trait40::get) const;

        Node() = default;
        int i = 40;
    };
};

}
