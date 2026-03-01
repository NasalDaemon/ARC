export module arc.bench.compile99_seq.node16;

import arc;
export import arc.bench.compile99_seq.trait.trait15;
export import arc.bench.compile99_seq.trait.trait16;

namespace arc::bench::compile99_seq {

export
struct Node16 : arc::Node
{
    using Depends = arc::Depends<Trait15>;
    using Traits = arc::Traits<Trait16>;

    int impl(this auto const& self, Trait16::get)
    {
        return self.i + self.getNode(trait15).get();
    }

    Node16() = default;
    int i = 16;
};

}
