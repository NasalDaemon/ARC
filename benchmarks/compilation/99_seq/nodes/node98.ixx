export module arc.bench.compile99_seq.node98;

import arc;
export import arc.bench.compile99_seq.trait.trait97;
export import arc.bench.compile99_seq.trait.trait98;

namespace arc::bench::compile99_seq {

export
struct Node98 : arc::Node
{
    using Depends = arc::Depends<trait::Trait97>;
    using Traits = arc::Traits<Node98, trait::Trait98>;

    int impl(this auto const& self, trait::Trait98::get)
    {
        return self.i + self.getNode(trait::trait97).get();
    }

    Node98() = default;
    int i = 98;
};

}
