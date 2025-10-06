module arc.bench.compile99.node26:impl;
import arc.bench.compile99.node26;

namespace arc::bench::compile99 {

template<class Context>
int Node26::Node<Context>::impl(trait::Trait26::get) const
{
    return i + getNode(trait::trait25).get();
}

} // namespace arc::bench::compile99
