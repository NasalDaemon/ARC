export module arc.bench.compile99_seq.node65;

import arc;
export import arc.bench.compile99_seq.trait.trait64;
export import arc.bench.compile99_seq.trait.trait65;

namespace arc::bench::compile99_seq {

export
struct Node65 : arc::Node
{
    using Depends = arc::Depends<trait::Trait64>;
    using Traits = arc::Traits<Node65, trait::Trait65>;

    int impl(this auto const& self, trait::Trait65::get)
    {
        return self.i + self.getNode(trait::trait64).get();
    }

    Node65() = default;
    int i = 65;
};

}
