module arc.bench.compile99.node81:impl;
import arc.bench.compile99.node81;

namespace arc::bench::compile99 {

template<class Context>
int Node81::Node<Context>::impl(Trait81::get) const
{
    return i + getNode(trait80).get();
}

}
