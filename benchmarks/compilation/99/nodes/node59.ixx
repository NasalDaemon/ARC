export module arc.bench.compile99.node59;

import arc;
export import arc.bench.compile99.trait.trait58;
export import arc.bench.compile99.trait.trait59;

namespace arc::bench::compile99 {

export
struct Node59
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait58>;
        using Traits  = arc::Traits<Trait59>;

        int impl(Trait59::get) const;

        Node() = default;
        int i = 59;
    };
};

}
