module arc.bench.compile99.node20:impl;
import arc.bench.compile99.node20;

namespace arc::bench::compile99 {
template<class Context>
int Node20::Node<Context>::impl(trait::Trait20::get) const
{
    return i + getNode(trait::trait19).get();
}
}
