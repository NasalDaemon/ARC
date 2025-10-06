export module arc.bench.compile99_seq.node68;

import arc;
export import arc.bench.compile99_seq.trait.trait67;
export import arc.bench.compile99_seq.trait.trait68;

namespace arc::bench::compile99_seq {

export
struct Node68 : arc::Node
{
    using Depends = arc::Depends<trait::Trait67>;
    using Traits = arc::Traits<Node68, trait::Trait68>;

    int impl(this auto const& self, trait::Trait68::get)
    {
        return self.i + self.getNode(trait::trait67).get();
    }

    Node68() = default;
    int i = 68;
};

}
