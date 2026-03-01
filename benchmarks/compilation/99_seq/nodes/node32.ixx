export module arc.bench.compile99_seq.node32;

import arc;
export import arc.bench.compile99_seq.trait.trait31;
export import arc.bench.compile99_seq.trait.trait32;

namespace arc::bench::compile99_seq {

export
struct Node32 : arc::Node
{
    using Depends = arc::Depends<Trait31>;
    using Traits = arc::Traits<Trait32>;

    int impl(this auto const& self, Trait32::get)
    {
        return self.i + self.getNode(trait31).get();
    }

    Node32() = default;
    int i = 32;
};

}
