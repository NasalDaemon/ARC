export module arc.bench.compile99.node45;

import arc;
export import arc.bench.compile99.trait.trait44;
export import arc.bench.compile99.trait.trait45;

namespace arc::bench::compile99 {

export
struct Node45
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait44>;
        using Traits  = arc::Traits<Node, trait::Trait45>;

        int impl(trait::Trait45::get) const;

        Node() = default;
        int i = 45;
    };
};

}
