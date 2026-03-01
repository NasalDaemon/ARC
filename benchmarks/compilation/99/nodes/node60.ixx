export module arc.bench.compile99.node60;

import arc;
export import arc.bench.compile99.trait.trait59;
export import arc.bench.compile99.trait.trait60;

namespace arc::bench::compile99 {

export
struct Node60
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<Trait59>;
        using Traits  = arc::Traits<Trait60>;

        int impl(Trait60::get) const;

        Node() = default;
        int i = 60;
    };
};

}
