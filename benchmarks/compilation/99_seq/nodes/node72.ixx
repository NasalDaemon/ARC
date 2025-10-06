export module arc.bench.compile99_seq.node72;

import arc;
export import arc.bench.compile99_seq.trait.trait71;
export import arc.bench.compile99_seq.trait.trait72;

namespace arc::bench::compile99_seq {

export
struct Node72 : arc::Node
{
    using Depends = arc::Depends<trait::Trait71>;
    using Traits = arc::Traits<Node72, trait::Trait72>;

    int impl(this auto const& self, trait::Trait72::get)
    {
        return self.i + self.getNode(trait::trait71).get();
    }

    Node72() = default;
    int i = 72;
};

}
