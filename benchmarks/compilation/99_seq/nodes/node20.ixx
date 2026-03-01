export module arc.bench.compile99_seq.node20;

import arc;
export import arc.bench.compile99_seq.trait.trait19;
export import arc.bench.compile99_seq.trait.trait20;

namespace arc::bench::compile99_seq {

export
struct Node20 : arc::Node
{
    using Depends = arc::Depends<Trait19>;
    using Traits = arc::Traits<Trait20>;

    int impl(this auto const& self, Trait20::get)
    {
        return self.i + self.getNode(trait19).get();
    }

    Node20() = default;
    int i = 20;
};

}
