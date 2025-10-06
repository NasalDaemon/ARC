export module arc.bench.compile99_seq.node83;

import arc;
export import arc.bench.compile99_seq.trait.trait82;
export import arc.bench.compile99_seq.trait.trait83;

namespace arc::bench::compile99_seq {

export
struct Node83 : arc::Node
{
    using Depends = arc::Depends<trait::Trait82>;
    using Traits = arc::Traits<Node83, trait::Trait83>;

    int impl(this auto const& self, trait::Trait83::get)
    {
        return self.i + self.getNode(trait::trait82).get();
    }

    Node83() = default;
    int i = 83;
};

}
