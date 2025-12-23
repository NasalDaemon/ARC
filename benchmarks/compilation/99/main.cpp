import arc.bench.compile99.graph;
import arc.bench.compile99.trait.trait99;

import arc;

using namespace arc::bench::compile99;

// Will collapse into int main() { return 0; } in LTO release build
int main()
{
    arc::Graph<Graph> g;
    if (g.node99.asTrait(trait::trait99).get() == 4950)
        return 0;
    return 1;
}
