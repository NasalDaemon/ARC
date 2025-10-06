module arc.bench.compile99.node22:impl;
import arc.bench.compile99.node22;

namespace arc::bench::compile99 {

template<class Context>
int Node22::Node<Context>::impl(trait::Trait22::get) const
{
    return i + getNode(trait::trait21).get();
}

}
