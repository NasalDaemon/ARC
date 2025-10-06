export module arc.bench.compile99.node38;

import arc;
export import arc.bench.compile99.trait.trait37;
export import arc.bench.compile99.trait.trait38;

namespace arc::bench::compile99 {

export
struct Node38
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait37>;
        using Traits  = arc::Traits<Node, trait::Trait38>;

        int impl(trait::Trait38::get) const;

        Node() = default;
        int i = 38;
    };
};

}
