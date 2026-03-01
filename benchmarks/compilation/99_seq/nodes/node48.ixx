export module arc.bench.compile99_seq.node48;

import arc;
export import arc.bench.compile99_seq.trait.trait47;
export import arc.bench.compile99_seq.trait.trait48;

namespace arc::bench::compile99_seq {

export
struct Node48 : arc::Node
{
    using Depends = arc::Depends<Trait47>;
    using Traits = arc::Traits<Trait48>;

    int impl(this auto const& self, Trait48::get)
    {
        return self.i + self.getNode(trait47).get();
    }

    Node48() = default;
    int i = 48;
};

}
