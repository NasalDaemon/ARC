export module arc.bench.compile99_seq.node36;

import arc;
export import arc.bench.compile99_seq.trait.trait35;
export import arc.bench.compile99_seq.trait.trait36;

namespace arc::bench::compile99_seq {

export
struct Node36 : arc::Node
{
    using Depends = arc::Depends<trait::Trait35>;
    using Traits = arc::Traits<Node36, trait::Trait36>;

    int impl(this auto const& self, trait::Trait36::get)
    {
        return self.i + self.getNode(trait::trait35).get();
    }

    Node36() = default;
    int i = 36;
};

}
