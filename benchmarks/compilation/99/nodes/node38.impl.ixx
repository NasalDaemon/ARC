module arc.bench.compile99.node38:impl;
import arc.bench.compile99.node38;

namespace arc::bench::compile99 {

template<class Context>
int Node38::Node<Context>::impl(Trait38::get) const
{
    return i + getNode(trait37).get();
}

}
