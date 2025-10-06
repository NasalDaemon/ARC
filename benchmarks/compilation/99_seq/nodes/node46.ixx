export module arc.bench.compile99_seq.node46;

import arc;
export import arc.bench.compile99_seq.trait.trait45;
export import arc.bench.compile99_seq.trait.trait46;

namespace arc::bench::compile99_seq {

export
struct Node46 : arc::Node
{
    using Depends = arc::Depends<trait::Trait45>;
    using Traits = arc::Traits<Node46, trait::Trait46>;

    int impl(this auto const& self, trait::Trait46::get)
    {
        return self.i + self.getNode(trait::trait45).get();
    }

    Node46() = default;
    int i = 46;
};

}
