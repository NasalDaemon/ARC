export module arc.bench.compile99_seq.node92;

import arc;
export import arc.bench.compile99_seq.trait.trait91;
export import arc.bench.compile99_seq.trait.trait92;

namespace arc::bench::compile99_seq {

export
struct Node92 : arc::Node
{
    using Depends = arc::Depends<Trait91>;
    using Traits = arc::Traits<Trait92>;

    int impl(this auto const& self, Trait92::get)
    {
        return self.i + self.getNode(trait91).get();
    }

    Node92() = default;
    int i = 92;
};

}
