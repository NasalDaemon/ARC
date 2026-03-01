export module arc.bench.compile99_seq.node33;

import arc;
export import arc.bench.compile99_seq.trait.trait32;
export import arc.bench.compile99_seq.trait.trait33;

namespace arc::bench::compile99_seq {

export
struct Node33 : arc::Node
{
    using Depends = arc::Depends<Trait32>;
    using Traits = arc::Traits<Trait33>;

    int impl(this auto const& self, Trait33::get)
    {
        return self.i + self.getNode(trait32).get();
    }

    Node33() = default;
    int i = 33;
};

}
