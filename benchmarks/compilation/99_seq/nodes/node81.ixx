export module arc.bench.compile99_seq.node81;

import arc;
export import arc.bench.compile99_seq.trait.trait80;
export import arc.bench.compile99_seq.trait.trait81;

namespace arc::bench::compile99_seq {

export
struct Node81 : arc::Node
{
    using Depends = arc::Depends<Trait80>;
    using Traits = arc::Traits<Trait81>;

    int impl(this auto const& self, Trait81::get)
    {
        return self.i + self.getNode(trait80).get();
    }

    Node81() = default;
    int i = 81;
};

}
