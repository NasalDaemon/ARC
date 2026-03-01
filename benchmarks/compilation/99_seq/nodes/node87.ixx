export module arc.bench.compile99_seq.node87;

import arc;
export import arc.bench.compile99_seq.trait.trait86;
export import arc.bench.compile99_seq.trait.trait87;

namespace arc::bench::compile99_seq {

export
struct Node87 : arc::Node
{
    using Depends = arc::Depends<Trait86>;
    using Traits = arc::Traits<Trait87>;

    int impl(this auto const& self, Trait87::get)
    {
        return self.i + self.getNode(trait86).get();
    }

    Node87() = default;
    int i = 87;
};

}
