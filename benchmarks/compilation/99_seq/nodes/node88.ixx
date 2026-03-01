export module arc.bench.compile99_seq.node88;

import arc;
export import arc.bench.compile99_seq.trait.trait87;
export import arc.bench.compile99_seq.trait.trait88;

namespace arc::bench::compile99_seq {

export
struct Node88 : arc::Node
{
    using Depends = arc::Depends<Trait87>;
    using Traits = arc::Traits<Trait88>;

    int impl(this auto const& self, Trait88::get)
    {
        return self.i + self.getNode(trait87).get();
    }

    Node88() = default;
    int i = 88;
};

}
