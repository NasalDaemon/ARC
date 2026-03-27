import examples.calculator.clusters;
import examples.calculator.traits;
import arc;

using namespace examples::calculator;

auto main() -> int
{
    arc::Graph<domain::Calculator> graph;
    return graph.repl->run();
}
