module arc.bench.compile99.node24:impl;
import arc.bench.compile99.node24;

namespace arc::bench::compile99 {

template<class Context>
int Node24::Node<Context>::impl(trait::Trait24::get) const
{
    return i + getNode(trait::trait23).get();
}

}
