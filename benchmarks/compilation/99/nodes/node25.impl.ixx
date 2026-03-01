module arc.bench.compile99.node25:impl;
import arc.bench.compile99.node25;

namespace arc::bench::compile99 {

template<class Context>
int Node25::Node<Context>::impl(Trait25::get) const
{
    return i + getNode(trait24).get();
}

}
