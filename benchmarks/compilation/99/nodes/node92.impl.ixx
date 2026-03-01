module arc.bench.compile99.node92:impl;
import arc.bench.compile99.node92;

namespace arc::bench::compile99 {

template<class Context>
int Node92::Node<Context>::impl(Trait92::get) const
{
    return i + getNode(trait91).get();
}

}
