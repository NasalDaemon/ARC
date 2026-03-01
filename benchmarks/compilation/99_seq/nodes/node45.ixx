export module arc.bench.compile99_seq.node45;

import arc;
export import arc.bench.compile99_seq.trait.trait44;
export import arc.bench.compile99_seq.trait.trait45;

namespace arc::bench::compile99_seq {

export
struct Node45 : arc::Node
{
    using Depends = arc::Depends<Trait44>;
    using Traits = arc::Traits<Trait45>;

    int impl(this auto const& self, Trait45::get)
    {
        return self.i + self.getNode(trait44).get();
    }

    Node45() = default;
    int i = 45;
};

}
