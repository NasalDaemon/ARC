export module arc.bench.compile99.node48;

import arc;
export import arc.bench.compile99.trait.trait47;
export import arc.bench.compile99.trait.trait48;

namespace arc::bench::compile99 {

export
struct Node48
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait47>;
        using Traits  = arc::Traits<Node, trait::Trait48>;

        int impl(trait::Trait48::get) const;

        Node() = default;
        int i = 48;
    };
};

}
