export module arc.bench.compile99_seq.node69;

import arc;
export import arc.bench.compile99_seq.trait.trait68;
export import arc.bench.compile99_seq.trait.trait69;

namespace arc::bench::compile99_seq {

export
struct Node69 : arc::Node
{
    using Depends = arc::Depends<trait::Trait68>;
    using Traits = arc::Traits<Node69, trait::Trait69>;

    int impl(this auto const& self, trait::Trait69::get)
    {
        return self.i + self.getNode(trait::trait68).get();
    }

    Node69() = default;
    int i = 69;
};

}
