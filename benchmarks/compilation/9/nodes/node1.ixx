export module arc.bench.compile9.node1;

export import arc.bench.compile9.trait.trait1;
import arc;

namespace arc::bench::compile9 {

export
struct Node1
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node, trait::Trait1>;

        int impl(trait::Trait1::get) const;

        Node() = default;
        int i = 1;
    };
};

}
