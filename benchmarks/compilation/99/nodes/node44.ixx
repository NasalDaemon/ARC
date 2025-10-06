export module arc.bench.compile99.node44;

import arc;
export import arc.bench.compile99.trait.trait43;
export import arc.bench.compile99.trait.trait44;

namespace arc::bench::compile99 {

export
struct Node44
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Trait43>;
        using Traits  = arc::Traits<Node, trait::Trait44>;

        int impl(trait::Trait44::get) const;

        Node() = default;
        int i = 44;
    };
};

}
