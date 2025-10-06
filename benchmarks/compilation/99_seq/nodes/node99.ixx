export module arc.bench.compile99_seq.node99;

import arc;
export import arc.bench.compile99_seq.trait.trait98;
export import arc.bench.compile99_seq.trait.trait99;

namespace arc::bench::compile99_seq {

export
struct Node99 : arc::Node
{
    using Depends = arc::Depends<trait::Trait98>;
    using Traits = arc::Traits<Node99, trait::Trait99>;

    int impl(this auto const& self, trait::Trait99::get)
    {
        return self.i + self.getNode(trait::trait98).get();
    }

    Node99() = default;
    int i = 99;
};

}
