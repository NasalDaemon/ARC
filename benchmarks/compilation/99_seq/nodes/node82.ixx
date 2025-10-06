export module arc.bench.compile99_seq.node82;

import arc;
export import arc.bench.compile99_seq.trait.trait81;
export import arc.bench.compile99_seq.trait.trait82;

namespace arc::bench::compile99_seq {

export
struct Node82 : arc::Node
{
    using Depends = arc::Depends<trait::Trait81>;
    using Traits = arc::Traits<Node82, trait::Trait82>;

    int impl(this auto const& self, trait::Trait82::get)
    {
        return self.i + self.getNode(trait::trait81).get();
    }

    Node82() = default;
    int i = 82;
};

}
