module arc.bench.compile99.node36:impl;
import arc.bench.compile99.node36;

namespace arc::bench::compile99 {

template<class Context>
int Node36::Node<Context>::impl(trait::Trait36::get) const
{
    return i + getNode(trait::trait35).get();
}

} // namespace arc::bench::compile99
