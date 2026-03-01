export module arc.bench.compile99_seq.node55;

import arc;
export import arc.bench.compile99_seq.trait.trait54;
export import arc.bench.compile99_seq.trait.trait55;

namespace arc::bench::compile99_seq {

export
struct Node55 : arc::Node
{
    using Depends = arc::Depends<Trait54>;
    using Traits = arc::Traits<Trait55>;

    int impl(this auto const& self, Trait55::get)
    {
        return self.i + self.getNode(trait54).get();
    }

    Node55() = default;
    int i = 55;
};

}
