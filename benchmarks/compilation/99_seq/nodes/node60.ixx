export module arc.bench.compile99_seq.node60;

import arc;
export import arc.bench.compile99_seq.trait.trait59;
export import arc.bench.compile99_seq.trait.trait60;

namespace arc::bench::compile99_seq {

export
struct Node60 : arc::Node
{
    using Depends = arc::Depends<trait::Trait59>;
    using Traits = arc::Traits<Node60, trait::Trait60>;

    int impl(this auto const& self, trait::Trait60::get)
    {
        return self.i + self.getNode(trait::trait59).get();
    }

    Node60() = default;
    int i = 60;
};

}
