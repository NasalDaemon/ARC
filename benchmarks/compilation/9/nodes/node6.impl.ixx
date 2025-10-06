module arc.bench.compile9.node6:impl;
import arc.bench.compile9.node6;

namespace arc::bench::compile9 {

template<class Context>
int Node6::Node<Context>::impl(trait::Trait6::get) const
{
    return i + getNode(trait::trait5).get();
}

}
