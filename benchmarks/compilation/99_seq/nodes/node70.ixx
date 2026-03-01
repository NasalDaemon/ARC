export module arc.bench.compile99_seq.node70;

import arc;
export import arc.bench.compile99_seq.trait.trait69;
export import arc.bench.compile99_seq.trait.trait70;

namespace arc::bench::compile99_seq {

export
struct Node70 : arc::Node
{
    using Depends = arc::Depends<Trait69>;
    using Traits = arc::Traits<Trait70>;

    int impl(this auto const& self, Trait70::get)
    {
        return self.i + self.getNode(trait69).get();
    }

    Node70() = default;
    int i = 70;
};

}
