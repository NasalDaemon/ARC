export module arc.bench.compile99_seq.node12;

import arc;
export import arc.bench.compile99_seq.trait.trait11;
export import arc.bench.compile99_seq.trait.trait12;

namespace arc::bench::compile99_seq {

export
struct Node12 : arc::Node
{
    using Depends = arc::Depends<Trait11>;
    using Traits = arc::Traits<Trait12>;

    int impl(this auto const& self, Trait12::get)
    {
        return self.i + self.getNode(trait11).get();
    }

    Node12() = default;
    int i = 12;
};

}
