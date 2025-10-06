export module arc.bench.compile99_seq.node35;

import arc;
export import arc.bench.compile99_seq.trait.trait34;
export import arc.bench.compile99_seq.trait.trait35;

namespace arc::bench::compile99_seq {

export
struct Node35 : arc::Node
{
    using Depends = arc::Depends<trait::Trait34>;
    using Traits = arc::Traits<Node35, trait::Trait35>;

    int impl(this auto const& self, trait::Trait35::get)
    {
        return self.i + self.getNode(trait::trait34).get();
    }

    Node35() = default;
    int i = 35;
};

}
