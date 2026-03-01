module arc.bench.compile99.node27:impl;
import arc.bench.compile99.node27;

namespace arc::bench::compile99 {

template<class Context>
int Node27::Node<Context>::impl(Trait27::get) const
{
    return i + getNode(trait26).get();
}

}
