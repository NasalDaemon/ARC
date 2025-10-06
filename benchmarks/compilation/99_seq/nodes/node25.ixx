export module arc.bench.compile99_seq.node25;

import arc;
export import arc.bench.compile99_seq.trait.trait24;
export import arc.bench.compile99_seq.trait.trait25;

namespace arc::bench::compile99_seq {

export
struct Node25 : arc::Node
{
    using Depends = arc::Depends<trait::Trait24>;
    using Traits = arc::Traits<Node25, trait::Trait25>;

    int impl(this auto const& self, trait::Trait25::get)
    {
        return self.i + self.getNode(trait::trait24).get();
    }

    Node25() = default;
    int i = 25;
};

}
