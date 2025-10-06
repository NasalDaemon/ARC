export module arc.bench.compile99_seq.node42;

import arc;
export import arc.bench.compile99_seq.trait.trait41;
export import arc.bench.compile99_seq.trait.trait42;

namespace arc::bench::compile99_seq {

export
struct Node42 : arc::Node
{
    using Depends = arc::Depends<trait::Trait41>;
    using Traits = arc::Traits<Node42, trait::Trait42>;

    int impl(this auto const& self, trait::Trait42::get)
    {
        return self.i + self.getNode(trait::trait41).get();
    }

    Node42() = default;
    int i = 42;
};

}
