export module arc.bench.compile99.node17;

import arc;
export import arc.bench.compile99.trait.trait16;
export import arc.bench.compile99.trait.trait17;

namespace arc::bench::compile99 {

export
struct Node17
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait16>;
        using Traits  = arc::Traits<Node, trait::Trait17>;

        int impl(trait::Trait17::get) const;

        Node() = default;
        int i = 17;
    };
};

}
