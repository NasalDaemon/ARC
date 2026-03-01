export module arc.bench.compile99.node40;

import arc;
export import arc.bench.compile99.trait.trait39;
export import arc.bench.compile99.trait.trait40;

namespace arc::bench::compile99 {

export
struct Node40
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait39>;
        using Traits  = arc::Traits<Trait40>;

        int impl(Trait40::get) const;

        Node() = default;
        int i = 40;
    };
};

}
