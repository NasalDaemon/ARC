module arc.bench.compile99.node21:impl;
import arc.bench.compile99.node21;

namespace arc::bench::compile99 {

template<class Context>
int Node21::Node<Context>::impl(trait::Trait21::get) const
{
    return i + getNode(trait::trait20).get();
}

}
