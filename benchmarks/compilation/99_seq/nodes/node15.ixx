export module arc.bench.compile99_seq.node15;

import arc;
export import arc.bench.compile99_seq.trait.trait14;
export import arc.bench.compile99_seq.trait.trait15;

namespace arc::bench::compile99_seq {

export
struct Node15 : arc::Node
{
    using Depends = arc::Depends<trait::Trait14>;
    using Traits = arc::Traits<Node15, trait::Trait15>;

    int impl(this auto const& self, trait::Trait15::get)
    {
        return self.i + self.getNode(trait::trait14).get();
    }

    Node15() = default;
    int i = 15;
};

}
