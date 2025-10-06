module arc.bench.compile99.node33:impl;
import arc.bench.compile99.node33;

namespace arc::bench::compile99 {

template<class Context>
int Node33::Node<Context>::impl(trait::Trait33::get) const
{
    return i + getNode(trait::trait32).get();
}

}
