export module arc.bench.compile99.node47;

import arc;
export import arc.bench.compile99.trait.trait46;
export import arc.bench.compile99.trait.trait47;

namespace arc::bench::compile99 {

export
struct Node47
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait46>;
        using Traits  = arc::Traits<Node, trait::Trait47>;

        int impl(trait::Trait47::get) const;

        Node() = default;
        int i = 47;
    };
};

}
