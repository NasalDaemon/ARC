export module arc.bench.compile99_seq.node56;

import arc;
export import arc.bench.compile99_seq.trait.trait55;
export import arc.bench.compile99_seq.trait.trait56;

namespace arc::bench::compile99_seq {

export
struct Node56 : arc::Node
{
    using Depends = arc::Depends<trait::Trait55>;
    using Traits = arc::Traits<Node56, trait::Trait56>;

    int impl(this auto const& self, trait::Trait56::get)
    {
        return self.i + self.getNode(trait::trait55).get();
    }

    Node56() = default;
    int i = 56;
};

}
