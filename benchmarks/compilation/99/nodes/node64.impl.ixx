module arc.bench.compile99.node64:impl;
import arc.bench.compile99.node64;

namespace arc::bench::compile99 {

template<class Context>
int Node64::Node<Context>::impl(trait::Trait64::get) const
{
    return i + getNode(trait::trait63).get();
}

} // namespace arc::bench::compile99
