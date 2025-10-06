export module arc.bench.compile99_seq.node2;

import arc;
export import arc.bench.compile99_seq.trait.trait1;
export import arc.bench.compile99_seq.trait.trait2;

namespace arc::bench::compile99_seq {

export
struct Node2 : arc::Node
{
    using Depends = arc::Depends<trait::Trait1>;
    using Traits = arc::Traits<Node2, trait::Trait2>;

    int impl(this auto const& self, trait::Trait2::get)
    {
        return self.i + self.getNode(trait::trait1).get();
    }

    Node2() = default;
    int i = 2;
};

}
