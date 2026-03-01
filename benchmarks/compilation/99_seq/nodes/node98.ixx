export module arc.bench.compile99_seq.node98;

import arc;
export import arc.bench.compile99_seq.trait.trait97;
export import arc.bench.compile99_seq.trait.trait98;

namespace arc::bench::compile99_seq {

export
struct Node98 : arc::Node
{
    using Depends = arc::Depends<Trait97>;
    using Traits = arc::Traits<Trait98>;

    int impl(this auto const& self, Trait98::get)
    {
        return self.i + self.getNode(trait97).get();
    }

    Node98() = default;
    int i = 98;
};

}
