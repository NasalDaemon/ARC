export module arc.bench.compile99_seq.node89;

import arc;
export import arc.bench.compile99_seq.trait.trait88;
export import arc.bench.compile99_seq.trait.trait89;

namespace arc::bench::compile99_seq {

export
struct Node89 : arc::Node
{
    using Depends = arc::Depends<Trait88>;
    using Traits = arc::Traits<Trait89>;

    int impl(this auto const& self, Trait89::get)
    {
        return self.i + self.getNode(trait88).get();
    }

    Node89() = default;
    int i = 89;
};

}
