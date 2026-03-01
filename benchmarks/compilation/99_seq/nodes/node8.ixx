export module arc.bench.compile99_seq.node8;

import arc;
export import arc.bench.compile99_seq.trait.trait7;
export import arc.bench.compile99_seq.trait.trait8;

namespace arc::bench::compile99_seq {

export
struct Node8 : arc::Node
{
    using Depends = arc::Depends<Trait7>;
    using Traits = arc::Traits<Trait8>;

    int impl(this auto const& self, Trait8::get)
    {
        return self.i + self.getNode(trait7).get();
    }

    Node8() = default;
    int i = 8;
};

}
