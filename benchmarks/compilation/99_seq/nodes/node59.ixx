export module arc.bench.compile99_seq.node59;

import arc;
export import arc.bench.compile99_seq.trait.trait58;
export import arc.bench.compile99_seq.trait.trait59;

namespace arc::bench::compile99_seq {

export
struct Node59 : arc::Node
{
    using Depends = arc::Depends<Trait58>;
    using Traits = arc::Traits<Trait59>;

    int impl(this auto const& self, Trait59::get)
    {
        return self.i + self.getNode(trait58).get();
    }

    Node59() = default;
    int i = 59;
};

}
