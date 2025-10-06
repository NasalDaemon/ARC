module arc.bench.compile99.node65:impl;
import arc.bench.compile99.node65;

namespace arc::bench::compile99 {

template<class Context>
int Node65::Node<Context>::impl(trait::Trait65::get) const
{
    return i + getNode(trait::trait64).get();
}

} // namespace arc::bench::compile99
