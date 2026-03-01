export module arc.bench.compile99_seq.node57;

import arc;
export import arc.bench.compile99_seq.trait.trait56;
export import arc.bench.compile99_seq.trait.trait57;

namespace arc::bench::compile99_seq {

export
struct Node57 : arc::Node
{
    using Depends = arc::Depends<Trait56>;
    using Traits = arc::Traits<Trait57>;

    int impl(this auto const& self, Trait57::get)
    {
        return self.i + self.getNode(trait56).get();
    }

    Node57() = default;
    int i = 57;
};

}
