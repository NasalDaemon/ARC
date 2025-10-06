export module arc.bench.compile99_seq.node12;

import arc;
export import arc.bench.compile99_seq.trait.trait11;
export import arc.bench.compile99_seq.trait.trait12;

namespace arc::bench::compile99_seq {

export
struct Node12 : arc::Node
{
    using Depends = arc::Depends<trait::Trait11>;
    using Traits = arc::Traits<Node12, trait::Trait12>;

    int impl(this auto const& self, trait::Trait12::get)
    {
        return self.i + self.getNode(trait::trait11).get();
    }

    Node12() = default;
    int i = 12;
};

}
