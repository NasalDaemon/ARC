module arc.bench.compile99.node13:impl;
import arc.bench.compile99.node13;

namespace arc::bench::compile99 {
template<class Context>
int Node13::Node<Context>::impl(trait::Trait13::get) const
{
    return i + getNode(trait::trait12).get();
}
}
