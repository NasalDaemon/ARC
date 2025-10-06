export module arc.bench.compile99_seq.node79;

import arc;
export import arc.bench.compile99_seq.trait.trait78;
export import arc.bench.compile99_seq.trait.trait79;

namespace arc::bench::compile99_seq {

export
struct Node79 : arc::Node
{
    using Depends = arc::Depends<trait::Trait78>;
    using Traits = arc::Traits<Node79, trait::Trait79>;

    int impl(this auto const& self, trait::Trait79::get)
    {
        return self.i + self.getNode(trait::trait78).get();
    }

    Node79() = default;
    int i = 79;
};

}
