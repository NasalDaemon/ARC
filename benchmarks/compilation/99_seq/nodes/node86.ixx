export module arc.bench.compile99_seq.node86;

import arc;
export import arc.bench.compile99_seq.trait.trait85;
export import arc.bench.compile99_seq.trait.trait86;

namespace arc::bench::compile99_seq {

export
struct Node86 : arc::Node
{
    using Depends = arc::Depends<trait::Trait85>;
    using Traits = arc::Traits<Node86, trait::Trait86>;

    int impl(this auto const& self, trait::Trait86::get)
    {
        return self.i + self.getNode(trait::trait85).get();
    }

    Node86() = default;
    int i = 86;
};

}
