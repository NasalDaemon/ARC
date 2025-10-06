module arc.bench.compile99.node61:impl;
import arc.bench.compile99.node61;

namespace arc::bench::compile99 {

template<class Context>
int Node61::Node<Context>::impl(trait::Trait61::get) const
{
    return i + getNode(trait::trait60).get();
}

}
