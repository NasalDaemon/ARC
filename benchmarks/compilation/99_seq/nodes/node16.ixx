export module arc.bench.compile99_seq.node16;

import arc;
export import arc.bench.compile99_seq.trait.trait15;
export import arc.bench.compile99_seq.trait.trait16;

namespace arc::bench::compile99_seq {

export
struct Node16 : arc::Node
{
    using Depends = arc::Depends<trait::Trait15>;
    using Traits = arc::Traits<Node16, trait::Trait16>;

    int impl(this auto const& self, trait::Trait16::get)
    {
        return self.i + self.getNode(trait::trait15).get();
    }

    Node16() = default;
    int i = 16;
};

}
