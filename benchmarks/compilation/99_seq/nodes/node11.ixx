export module arc.bench.compile99_seq.node11;

import arc;
export import arc.bench.compile99_seq.trait.trait10;
export import arc.bench.compile99_seq.trait.trait11;

namespace arc::bench::compile99_seq {

export
struct Node11 : arc::Node
{
    using Depends = arc::Depends<trait::Trait10>;
    using Traits = arc::Traits<Node11, trait::Trait11>;

    int impl(this auto const& self, trait::Trait11::get)
    {
        return self.i + self.getNode(trait::trait10).get();
    }

    Node11() = default;
    int i = 11;
};

}
