export module arc.bench.compile99.node35;

import arc;
export import arc.bench.compile99.trait.trait34;
export import arc.bench.compile99.trait.trait35;

namespace arc::bench::compile99 {

export
struct Node35
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait34>;
        using Traits  = arc::Traits<Trait35>;

        int impl(Trait35::get) const;

        Node() = default;
        int i = 35;
    };
};

}
