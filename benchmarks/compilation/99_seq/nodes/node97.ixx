export module arc.bench.compile99_seq.node97;

import arc;
export import arc.bench.compile99_seq.trait.trait96;
export import arc.bench.compile99_seq.trait.trait97;

namespace arc::bench::compile99_seq {

export
struct Node97 : arc::Node
{
    using Depends = arc::Depends<Trait96>;
    using Traits = arc::Traits<Trait97>;

    int impl(this auto const& self, Trait97::get)
    {
        return self.i + self.getNode(trait96).get();
    }

    Node97() = default;
    int i = 97;
};

}
