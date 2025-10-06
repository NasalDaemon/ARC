export module arc.bench.compile99.node36;

import arc;
export import arc.bench.compile99.trait.trait35;
export import arc.bench.compile99.trait.trait36;

namespace arc::bench::compile99 {

export
struct Node36
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait35>;
        using Traits  = arc::Traits<Node, trait::Trait36>;

        int impl(trait::Trait36::get) const;

        Node() = default;
        int i = 36;
    };
};

}
