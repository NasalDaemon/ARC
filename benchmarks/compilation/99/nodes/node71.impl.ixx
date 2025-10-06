module arc.bench.compile99.node71:impl;
import arc.bench.compile99.node71;

namespace arc::bench::compile99 {

template<class Context>
int Node71::Node<Context>::impl(trait::Trait71::get) const
{
    return i + getNode(trait::trait70).get();
}

} // namespace arc::bench::compile99
