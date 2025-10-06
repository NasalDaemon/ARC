export module arc.bench.compile99.node60;

import arc;
export import arc.bench.compile99.trait.trait59;
export import arc.bench.compile99.trait.trait60;

namespace arc::bench::compile99 {

export
struct Node60
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait59>;
        using Traits  = arc::Traits<Node, trait::Trait60>;

        int impl(trait::Trait60::get) const;

        Node() = default;
        int i = 60;
    };
};

}
