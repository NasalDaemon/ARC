module arc.bench.compile99.node10:impl;
import arc.bench.compile99.node10;

namespace arc::bench::compile99 {

template<class Context>
int Node10::Node<Context>::impl(trait::Trait10::get) const
{
    return i + getNode(trait::trait9).get();
}

}
