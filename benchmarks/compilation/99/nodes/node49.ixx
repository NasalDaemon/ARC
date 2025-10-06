export module arc.bench.compile99.node49;

import arc;
export import arc.bench.compile99.trait.trait48;
export import arc.bench.compile99.trait.trait49;

namespace arc::bench::compile99 {

export
struct Node49
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait48>;
        using Traits  = arc::Traits<Node, trait::Trait49>;

        int impl(trait::Trait49::get) const;

        Node() = default;
        int i = 49;
    };
};

}
