module arc.bench.compile9.node7:impl;
import arc.bench.compile9.node7;

namespace arc::bench::compile9 {

template<class Context>
int Node7::Node<Context>::impl(trait::Trait7::get) const
{
    return i + getNode(trait::trait6).get();
}

}
