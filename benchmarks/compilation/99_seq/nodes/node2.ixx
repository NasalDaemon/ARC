export module arc.bench.compile99_seq.node2;

import arc;
export import arc.bench.compile99_seq.trait.trait1;
export import arc.bench.compile99_seq.trait.trait2;

namespace arc::bench::compile99_seq {

export
struct Node2 : arc::Node
{
    using Depends = arc::Depends<Trait1>;
    using Traits = arc::Traits<Trait2>;

    int impl(this auto const& self, Trait2::get)
    {
        return self.i + self.getNode(trait1).get();
    }

    Node2() = default;
    int i = 2;
};

}
