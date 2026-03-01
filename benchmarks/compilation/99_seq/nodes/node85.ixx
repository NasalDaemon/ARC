export module arc.bench.compile99_seq.node85;

import arc;
export import arc.bench.compile99_seq.trait.trait84;
export import arc.bench.compile99_seq.trait.trait85;

namespace arc::bench::compile99_seq {

export
struct Node85 : arc::Node
{
    using Depends = arc::Depends<Trait84>;
    using Traits = arc::Traits<Trait85>;

    int impl(this auto const& self, Trait85::get)
    {
        return self.i + self.getNode(trait84).get();
    }

    Node85() = default;
    int i = 85;
};

}
