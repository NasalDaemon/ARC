export module arc.bench.compile99_seq.node85;

import arc;
export import arc.bench.compile99_seq.trait.trait84;
export import arc.bench.compile99_seq.trait.trait85;

namespace arc::bench::compile99_seq {

export
struct Node85 : arc::Node
{
    using Depends = arc::Depends<trait::Trait84>;
    using Traits = arc::Traits<Node85, trait::Trait85>;

    int impl(this auto const& self, trait::Trait85::get)
    {
        return self.i + self.getNode(trait::trait84).get();
    }

    Node85() = default;
    int i = 85;
};

}
