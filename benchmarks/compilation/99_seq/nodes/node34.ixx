export module arc.bench.compile99_seq.node34;

import arc;
export import arc.bench.compile99_seq.trait.trait33;
export import arc.bench.compile99_seq.trait.trait34;

namespace arc::bench::compile99_seq {

export
struct Node34 : arc::Node
{
    using Depends = arc::Depends<trait::Trait33>;
    using Traits = arc::Traits<Node34, trait::Trait34>;

    int impl(this auto const& self, trait::Trait34::get)
    {
        return self.i + self.getNode(trait::trait33).get();
    }

    Node34() = default;
    int i = 34;
};

}
