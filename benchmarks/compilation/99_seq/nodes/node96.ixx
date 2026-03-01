export module arc.bench.compile99_seq.node96;

import arc;
export import arc.bench.compile99_seq.trait.trait95;
export import arc.bench.compile99_seq.trait.trait96;

namespace arc::bench::compile99_seq {

export
struct Node96 : arc::Node
{
    using Depends = arc::Depends<Trait95>;
    using Traits = arc::Traits<Trait96>;

    int impl(this auto const& self, Trait96::get)
    {
        return self.i + self.getNode(trait95).get();
    }

    Node96() = default;
    int i = 96;
};

}
