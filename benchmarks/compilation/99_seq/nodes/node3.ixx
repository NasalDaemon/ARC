export module arc.bench.compile99_seq.node3;

import arc;
export import arc.bench.compile99_seq.trait.trait2;
export import arc.bench.compile99_seq.trait.trait3;

namespace arc::bench::compile99_seq {

export
struct Node3 : arc::Node
{
    using Depends = arc::Depends<Trait2>;
    using Traits = arc::Traits<Trait3>;

    int impl(this auto const& self, Trait3::get)
    {
        return self.i + self.getNode(trait2).get();
    }

    Node3() = default;
    int i = 3;
};

}
