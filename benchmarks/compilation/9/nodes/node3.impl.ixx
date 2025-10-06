module arc.bench.compile9.node3:impl;
import arc.bench.compile9.node3;

namespace arc::bench::compile9 {

template<class Context>
int Node3::Node<Context>::impl(trait::Trait3::get) const
{
    return i + getNode(trait::trait2).get();
}

}
