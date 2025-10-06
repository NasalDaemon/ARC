module arc.bench.compile99.node42:impl;
import arc.bench.compile99.node42;

namespace arc::bench::compile99 {

template<class Context>
int Node42::Node<Context>::impl(trait::Trait42::get) const
{
    return i + getNode(trait::trait41).get();
}

}
