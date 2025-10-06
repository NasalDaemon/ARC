export module arc.bench.compile99_seq.node89;

import arc;
export import arc.bench.compile99_seq.trait.trait88;
export import arc.bench.compile99_seq.trait.trait89;

namespace arc::bench::compile99_seq {

export
struct Node89 : arc::Node
{
    using Depends = arc::Depends<trait::Trait88>;
    using Traits = arc::Traits<Node89, trait::Trait89>;

    int impl(this auto const& self, trait::Trait89::get)
    {
        return self.i + self.getNode(trait::trait88).get();
    }

    Node89() = default;
    int i = 89;
};

}
