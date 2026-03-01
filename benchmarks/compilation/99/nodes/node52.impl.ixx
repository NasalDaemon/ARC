module arc.bench.compile99.node52:impl;
import arc.bench.compile99.node52;

namespace arc::bench::compile99 {

template<class Context>
int Node52::Node<Context>::impl(Trait52::get) const
{
    return i + getNode(trait51).get();
}

}
