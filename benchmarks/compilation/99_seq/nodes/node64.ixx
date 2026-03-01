export module arc.bench.compile99_seq.node64;

import arc;
export import arc.bench.compile99_seq.trait.trait63;
export import arc.bench.compile99_seq.trait.trait64;

namespace arc::bench::compile99_seq {

export
struct Node64 : arc::Node
{
    using Depends = arc::Depends<Trait63>;
    using Traits = arc::Traits<Trait64>;

    int impl(this auto const& self, Trait64::get)
    {
        return self.i + self.getNode(trait63).get();
    }

    Node64() = default;
    int i = 64;
};

}
