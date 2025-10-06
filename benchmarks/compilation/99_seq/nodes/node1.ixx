export module arc.bench.compile99_seq.node1;

export import arc.bench.compile99_seq.trait.trait1;
import arc;

namespace arc::bench::compile99_seq {

export
struct Node1 : arc::Node
{
    using Traits = arc::Traits<Node1, trait::Trait1>;

    int impl(trait::Trait1::get) const
    {
        return i;
    }

    Node1() = default;
    int i = 1;
};

}
