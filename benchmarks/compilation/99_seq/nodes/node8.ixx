export module arc.bench.compile99_seq.node8;

import arc;
export import arc.bench.compile99_seq.trait.trait7;
export import arc.bench.compile99_seq.trait.trait8;

namespace arc::bench::compile99_seq {

export
struct Node8 : arc::Node
{
    using Depends = arc::Depends<trait::Trait7>;
    using Traits = arc::Traits<Node8, trait::Trait8>;

    int impl(this auto const& self, trait::Trait8::get)
    {
        return self.i + self.getNode(trait::trait7).get();
    }

    Node8() = default;
    int i = 8;
};

}
