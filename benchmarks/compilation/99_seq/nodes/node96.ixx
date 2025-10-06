export module arc.bench.compile99_seq.node96;

import arc;
export import arc.bench.compile99_seq.trait.trait95;
export import arc.bench.compile99_seq.trait.trait96;

namespace arc::bench::compile99_seq {

export
struct Node96 : arc::Node
{
    using Depends = arc::Depends<trait::Trait95>;
    using Traits = arc::Traits<Node96, trait::Trait96>;

    int impl(this auto const& self, trait::Trait96::get)
    {
        return self.i + self.getNode(trait::trait95).get();
    }

    Node96() = default;
    int i = 96;
};

}
