export module arc.bench.compile99_seq.node90;

import arc;
export import arc.bench.compile99_seq.trait.trait89;
export import arc.bench.compile99_seq.trait.trait90;

namespace arc::bench::compile99_seq {

export
struct Node90 : arc::Node
{
    using Depends = arc::Depends<Trait89>;
    using Traits = arc::Traits<Trait90>;

    int impl(this auto const& self, Trait90::get)
    {
        return self.i + self.getNode(trait89).get();
    }

    Node90() = default;
    int i = 90;
};

}
