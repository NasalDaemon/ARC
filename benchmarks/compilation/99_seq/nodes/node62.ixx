export module arc.bench.compile99_seq.node62;

import arc;
export import arc.bench.compile99_seq.trait.trait61;
export import arc.bench.compile99_seq.trait.trait62;

namespace arc::bench::compile99_seq {

export
struct Node62 : arc::Node
{
    using Depends = arc::Depends<Trait61>;
    using Traits = arc::Traits<Trait62>;

    int impl(this auto const& self, Trait62::get)
    {
        return self.i + self.getNode(trait61).get();
    }

    Node62() = default;
    int i = 62;
};

}
