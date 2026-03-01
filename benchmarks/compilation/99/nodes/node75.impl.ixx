module arc.bench.compile99.node75:impl;
import arc.bench.compile99.node75;

namespace arc::bench::compile99 {

template<class Context>
int Node75::Node<Context>::impl(Trait75::get) const
{
    return i + getNode(trait74).get();
}

}
