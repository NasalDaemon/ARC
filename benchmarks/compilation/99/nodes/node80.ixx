export module arc.bench.compile99.node80;

import arc;
export import arc.bench.compile99.trait.trait79;
export import arc.bench.compile99.trait.trait80;

namespace arc::bench::compile99 {

export
struct Node80
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait79>;
        using Traits  = arc::Traits<Node, trait::Trait80>;

        int impl(trait::Trait80::get) const;

        Node() = default;
        int i = 80;
    };
};

}
