export module arc.bench.compile99_seq.node19;

import arc;
export import arc.bench.compile99_seq.trait.trait18;
export import arc.bench.compile99_seq.trait.trait19;

namespace arc::bench::compile99_seq {

export
struct Node19 : arc::Node
{
    using Depends = arc::Depends<Trait18>;
    using Traits = arc::Traits<Trait19>;

    int impl(this auto const& self, Trait19::get)
    {
        return self.i + self.getNode(trait18).get();
    }

    Node19() = default;
    int i = 19;
};

}
