export module arc.bench.compile99_seq.node37;

import arc;
export import arc.bench.compile99_seq.trait.trait36;
export import arc.bench.compile99_seq.trait.trait37;

namespace arc::bench::compile99_seq {

export
struct Node37 : arc::Node
{
    using Depends = arc::Depends<Trait36>;
    using Traits = arc::Traits<Trait37>;

    int impl(this auto const& self, Trait37::get)
    {
        return self.i + self.getNode(trait36).get();
    }

    Node37() = default;
    int i = 37;
};

}
