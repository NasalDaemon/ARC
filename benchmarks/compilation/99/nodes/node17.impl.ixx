module arc.bench.compile99.node17:impl;
import arc.bench.compile99.node17;

namespace arc::bench::compile99 {
template<class Context>
int Node17::Node<Context>::impl(Trait17::get) const
{
    return i + getNode(trait16).get();
}
}
