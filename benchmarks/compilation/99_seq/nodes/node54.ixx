export module arc.bench.compile99_seq.node54;

import arc;
export import arc.bench.compile99_seq.trait.trait53;
export import arc.bench.compile99_seq.trait.trait54;

namespace arc::bench::compile99_seq {

export
struct Node54 : arc::Node
{
    using Depends = arc::Depends<trait::Trait53>;
    using Traits = arc::Traits<Node54, trait::Trait54>;

    int impl(this auto const& self, trait::Trait54::get)
    {
        return self.i + self.getNode(trait::trait53).get();
    }

    Node54() = default;
    int i = 54;
};

}
