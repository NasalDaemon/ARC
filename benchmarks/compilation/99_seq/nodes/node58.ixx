export module arc.bench.compile99_seq.node58;

import arc;
export import arc.bench.compile99_seq.trait.trait57;
export import arc.bench.compile99_seq.trait.trait58;

namespace arc::bench::compile99_seq {

export
struct Node58 : arc::Node
{
    using Depends = arc::Depends<trait::Trait57>;
    using Traits = arc::Traits<Node58, trait::Trait58>;

    int impl(this auto const& self, trait::Trait58::get)
    {
        return self.i + self.getNode(trait::trait57).get();
    }

    Node58() = default;
    int i = 58;
};

}
