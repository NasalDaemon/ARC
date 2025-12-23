import arc.bench.compile9.graph;
import arc.bench.compile9.trait.trait9;

import arc;

using namespace arc::bench::compile9;

// Will collapse into int main() { return 0; } in LTO release build
int main()
{
    arc::Graph<Graph> g;
    if (g.node9.asTrait(trait::trait9).get() == 45)
        return 0;
    return 1;
}
