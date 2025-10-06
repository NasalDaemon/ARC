export module arc.bench.compile99.node13;

import arc;
export import arc.bench.compile99.trait.trait12;
export import arc.bench.compile99.trait.trait13;

namespace arc::bench::compile99 {

export
struct Node13
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait12>;
        using Traits  = arc::Traits<Node, trait::Trait13>;

        int impl(trait::Trait13::get) const;

        Node() = default;
        int i = 13;
    };
};

}
