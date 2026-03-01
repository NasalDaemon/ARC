module arc.bench.compile99.node13:impl;
import arc.bench.compile99.node13;

namespace arc::bench::compile99 {
template<class Context>
int Node13::Node<Context>::impl(Trait13::get) const
{
    return i + getNode(trait12).get();
}
}
