export module arc.bench.compile99_seq.node74;

import arc;
export import arc.bench.compile99_seq.trait.trait73;
export import arc.bench.compile99_seq.trait.trait74;

namespace arc::bench::compile99_seq {

export
struct Node74 : arc::Node
{
    using Depends = arc::Depends<Trait73>;
    using Traits = arc::Traits<Trait74>;

    int impl(this auto const& self, Trait74::get)
    {
        return self.i + self.getNode(trait73).get();
    }

    Node74() = default;
    int i = 74;
};

}
