export module arc.bench.compile99.node59;

import arc;
export import arc.bench.compile99.trait.trait58;
export import arc.bench.compile99.trait.trait59;

namespace arc::bench::compile99 {

export
struct Node59
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait58>;
        using Traits  = arc::Traits<Node, trait::Trait59>;

        int impl(trait::Trait59::get) const;

        Node() = default;
        int i = 59;
    };
};

}
