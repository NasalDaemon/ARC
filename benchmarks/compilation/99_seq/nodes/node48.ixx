export module arc.bench.compile99_seq.node48;

import arc;
export import arc.bench.compile99_seq.trait.trait47;
export import arc.bench.compile99_seq.trait.trait48;

namespace arc::bench::compile99_seq {

export
struct Node48 : arc::Node
{
    using Depends = arc::Depends<trait::Trait47>;
    using Traits = arc::Traits<Node48, trait::Trait48>;

    int impl(this auto const& self, trait::Trait48::get)
    {
        return self.i + self.getNode(trait::trait47).get();
    }

    Node48() = default;
    int i = 48;
};

}
