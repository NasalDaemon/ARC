export module arc.bench.compile99_seq.node82;

import arc;
export import arc.bench.compile99_seq.trait.trait81;
export import arc.bench.compile99_seq.trait.trait82;

namespace arc::bench::compile99_seq {

export
struct Node82 : arc::Node
{
    using Depends = arc::Depends<Trait81>;
    using Traits = arc::Traits<Trait82>;

    int impl(this auto const& self, Trait82::get)
    {
        return self.i + self.getNode(trait81).get();
    }

    Node82() = default;
    int i = 82;
};

}
