export module arc.bench.compile99_seq.node94;

import arc;
export import arc.bench.compile99_seq.trait.trait93;
export import arc.bench.compile99_seq.trait.trait94;

namespace arc::bench::compile99_seq {

export
struct Node94 : arc::Node
{
    using Depends = arc::Depends<Trait93>;
    using Traits = arc::Traits<Trait94>;

    int impl(this auto const& self, Trait94::get)
    {
        return self.i + self.getNode(trait93).get();
    }

    Node94() = default;
    int i = 94;
};

}
