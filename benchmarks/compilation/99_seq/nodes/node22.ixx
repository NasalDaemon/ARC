export module arc.bench.compile99_seq.node22;

import arc;
export import arc.bench.compile99_seq.trait.trait21;
export import arc.bench.compile99_seq.trait.trait22;

namespace arc::bench::compile99_seq {

export
struct Node22 : arc::Node
{
    using Depends = arc::Depends<Trait21>;
    using Traits = arc::Traits<Trait22>;

    int impl(this auto const& self, Trait22::get)
    {
        return self.i + self.getNode(trait21).get();
    }

    Node22() = default;
    int i = 22;
};

}
