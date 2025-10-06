module arc.bench.compile99.node17:impl;
import arc.bench.compile99.node17;

namespace arc::bench::compile99 {
template<class Context>
int Node17::Node<Context>::impl(trait::Trait17::get) const
{
    return i + getNode(trait::trait16).get();
}
}
