export module arc.bench.compile99_seq.node63;

import arc;
export import arc.bench.compile99_seq.trait.trait62;
export import arc.bench.compile99_seq.trait.trait63;

namespace arc::bench::compile99_seq {

export
struct Node63 : arc::Node
{
    using Depends = arc::Depends<trait::Trait62>;
    using Traits = arc::Traits<Node63, trait::Trait63>;

    int impl(this auto const& self, trait::Trait63::get)
    {
        return self.i + self.getNode(trait::trait62).get();
    }

    Node63() = default;
    int i = 63;
};

}
