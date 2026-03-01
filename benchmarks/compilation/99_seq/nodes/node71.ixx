export module arc.bench.compile99_seq.node71;

import arc;
export import arc.bench.compile99_seq.trait.trait70;
export import arc.bench.compile99_seq.trait.trait71;

namespace arc::bench::compile99_seq {

export
struct Node71 : arc::Node
{
    using Depends = arc::Depends<Trait70>;
    using Traits = arc::Traits<Trait71>;

    int impl(this auto const& self, Trait71::get)
    {
        return self.i + self.getNode(trait70).get();
    }

    Node71() = default;
    int i = 71;
};

}
