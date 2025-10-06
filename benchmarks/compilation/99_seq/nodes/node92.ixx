export module arc.bench.compile99_seq.node92;

import arc;
export import arc.bench.compile99_seq.trait.trait91;
export import arc.bench.compile99_seq.trait.trait92;

namespace arc::bench::compile99_seq {

export
struct Node92 : arc::Node
{
    using Depends = arc::Depends<trait::Trait91>;
    using Traits = arc::Traits<Node92, trait::Trait92>;

    int impl(this auto const& self, trait::Trait92::get)
    {
        return self.i + self.getNode(trait::trait91).get();
    }

    Node92() = default;
    int i = 92;
};

}
