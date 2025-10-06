export module arc.bench.compile99_seq.node81;

import arc;
export import arc.bench.compile99_seq.trait.trait80;
export import arc.bench.compile99_seq.trait.trait81;

namespace arc::bench::compile99_seq {

export
struct Node81 : arc::Node
{
    using Depends = arc::Depends<trait::Trait80>;
    using Traits = arc::Traits<Node81, trait::Trait81>;

    int impl(this auto const& self, trait::Trait81::get)
    {
        return self.i + self.getNode(trait::trait80).get();
    }

    Node81() = default;
    int i = 81;
};

}
