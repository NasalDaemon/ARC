export module arc.bench.compile99_seq.node66;

import arc;
export import arc.bench.compile99_seq.trait.trait65;
export import arc.bench.compile99_seq.trait.trait66;

namespace arc::bench::compile99_seq {

export
struct Node66 : arc::Node
{
    using Depends = arc::Depends<trait::Trait65>;
    using Traits = arc::Traits<Node66, trait::Trait66>;

    int impl(this auto const& self, trait::Trait66::get)
    {
        return self.i + self.getNode(trait::trait65).get();
    }

    Node66() = default;
    int i = 66;
};

}
