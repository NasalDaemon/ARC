module arc.bench.compile99.node14:impl;
import arc.bench.compile99.node14;

namespace arc::bench::compile99 {
template<class Context>
int Node14::Node<Context>::impl(trait::Trait14::get) const
{
    return i + getNode(trait::trait13).get();
}
}
