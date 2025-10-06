module arc.bench.compile99.node91:impl;
import arc.bench.compile99.node91;

namespace arc::bench::compile99 {

template<class Context>
int Node91::Node<Context>::impl(trait::Trait91::get) const
{
    return i + getNode(trait::trait90).get();
}

}
