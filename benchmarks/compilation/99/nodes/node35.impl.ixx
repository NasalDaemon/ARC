module arc.bench.compile99.node35:impl;
import arc.bench.compile99.node35;

namespace arc::bench::compile99 {

template<class Context>
int Node35::Node<Context>::impl(Trait35::get) const
{
    return i + getNode(trait34).get();
}

}
