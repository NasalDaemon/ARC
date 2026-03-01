export module arc.bench.compile99_seq.node17;

import arc;
export import arc.bench.compile99_seq.trait.trait16;
export import arc.bench.compile99_seq.trait.trait17;

namespace arc::bench::compile99_seq {

export
struct Node17 : arc::Node
{
    using Depends = arc::Depends<Trait16>;
    using Traits = arc::Traits<Trait17>;

    int impl(this auto const& self, Trait17::get)
    {
        return self.i + self.getNode(trait16).get();
    }

    Node17() = default;
    int i = 17;
};

}
