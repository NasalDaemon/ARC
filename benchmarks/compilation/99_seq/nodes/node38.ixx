export module arc.bench.compile99_seq.node38;

import arc;
export import arc.bench.compile99_seq.trait.trait37;
export import arc.bench.compile99_seq.trait.trait38;

namespace arc::bench::compile99_seq {

export
struct Node38 : arc::Node
{
    using Depends = arc::Depends<trait::Trait37>;
    using Traits = arc::Traits<Node38, trait::Trait38>;

    int impl(this auto const& self, trait::Trait38::get)
    {
        return self.i + self.getNode(trait::trait37).get();
    }

    Node38() = default;
    int i = 38;
};

}
