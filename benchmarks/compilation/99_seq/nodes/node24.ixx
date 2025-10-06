export module arc.bench.compile99_seq.node24;

import arc;
export import arc.bench.compile99_seq.trait.trait23;
export import arc.bench.compile99_seq.trait.trait24;

namespace arc::bench::compile99_seq {

export
struct Node24 : arc::Node
{
    using Depends = arc::Depends<trait::Trait23>;
    using Traits = arc::Traits<Node24, trait::Trait24>;

    int impl(this auto const& self, trait::Trait24::get)
    {
        return self.i + self.getNode(trait::trait23).get();
    }

    Node24() = default;
    int i = 24;
};

}
