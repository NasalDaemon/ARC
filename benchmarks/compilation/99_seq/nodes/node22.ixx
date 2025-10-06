export module arc.bench.compile99_seq.node22;

import arc;
export import arc.bench.compile99_seq.trait.trait21;
export import arc.bench.compile99_seq.trait.trait22;

namespace arc::bench::compile99_seq {

export
struct Node22 : arc::Node
{
    using Depends = arc::Depends<trait::Trait21>;
    using Traits = arc::Traits<Node22, trait::Trait22>;

    int impl(this auto const& self, trait::Trait22::get)
    {
        return self.i + self.getNode(trait::trait21).get();
    }

    Node22() = default;
    int i = 22;
};

}
