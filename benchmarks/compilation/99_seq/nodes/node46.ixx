export module arc.bench.compile99_seq.node46;

import arc;
export import arc.bench.compile99_seq.trait.trait45;
export import arc.bench.compile99_seq.trait.trait46;

namespace arc::bench::compile99_seq {

export
struct Node46 : arc::Node
{
    using Depends = arc::Depends<Trait45>;
    using Traits = arc::Traits<Trait46>;

    int impl(this auto const& self, Trait46::get)
    {
        return self.i + self.getNode(trait45).get();
    }

    Node46() = default;
    int i = 46;
};

}
