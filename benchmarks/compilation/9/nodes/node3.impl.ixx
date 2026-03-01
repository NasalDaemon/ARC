module arc.bench.compile9.node3:impl;
import arc.bench.compile9.node3;

namespace arc::bench::compile9 {

template<class Context>
int Node3::Node<Context>::impl(Trait3::get) const
{
    return i + getNode(trait2).get();
}

}
