export module arc.bench.compile99_seq.node70;

import arc;
export import arc.bench.compile99_seq.trait.trait69;
export import arc.bench.compile99_seq.trait.trait70;

namespace arc::bench::compile99_seq {

export
struct Node70 : arc::Node
{
    using Depends = arc::Depends<trait::Trait69>;
    using Traits = arc::Traits<Node70, trait::Trait70>;

    int impl(this auto const& self, trait::Trait70::get)
    {
        return self.i + self.getNode(trait::trait69).get();
    }

    Node70() = default;
    int i = 70;
};

}
