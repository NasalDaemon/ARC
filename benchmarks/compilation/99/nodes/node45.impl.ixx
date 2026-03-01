module arc.bench.compile99.node45:impl;
import arc.bench.compile99.node45;

namespace arc::bench::compile99 {

template<class Context>
int Node45::Node<Context>::impl(Trait45::get) const
{
    return i + getNode(trait44).get();
}

}
