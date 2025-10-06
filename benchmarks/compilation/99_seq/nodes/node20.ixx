export module arc.bench.compile99_seq.node20;

import arc;
export import arc.bench.compile99_seq.trait.trait19;
export import arc.bench.compile99_seq.trait.trait20;

namespace arc::bench::compile99_seq {

export
struct Node20 : arc::Node
{
    using Depends = arc::Depends<trait::Trait19>;
    using Traits = arc::Traits<Node20, trait::Trait20>;

    int impl(this auto const& self, trait::Trait20::get)
    {
        return self.i + self.getNode(trait::trait19).get();
    }

    Node20() = default;
    int i = 20;
};

}
