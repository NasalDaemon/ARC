export module arc.bench.compile99_seq.node43;

import arc;
export import arc.bench.compile99_seq.trait.trait42;
export import arc.bench.compile99_seq.trait.trait43;

namespace arc::bench::compile99_seq {

export
struct Node43 : arc::Node
{
    using Depends = arc::Depends<trait::Trait42>;
    using Traits = arc::Traits<Node43, trait::Trait43>;

    int impl(this auto const& self, trait::Trait43::get)
    {
        return self.i + self.getNode(trait::trait42).get();
    }

    Node43() = default;
    int i = 43;
};

}
