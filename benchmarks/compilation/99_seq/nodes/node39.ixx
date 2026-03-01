export module arc.bench.compile99_seq.node39;

import arc;
export import arc.bench.compile99_seq.trait.trait38;
export import arc.bench.compile99_seq.trait.trait39;

namespace arc::bench::compile99_seq {

export
struct Node39 : arc::Node
{
    using Depends = arc::Depends<Trait38>;
    using Traits = arc::Traits<Trait39>;

    int impl(this auto const& self, Trait39::get)
    {
        return self.i + self.getNode(trait38).get();
    }

    Node39() = default;
    int i = 39;
};

}
