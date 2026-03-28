import examples.calculator.graphs;
import arc;
import std;

using namespace examples::calculator;

auto main(int argc, char* argv[]) -> int
{
    if (argc > 1)
    {
        graph::Batch graph;
        std::vector<std::string> inputs;
        for (int i = 1; i < argc; ++i)
            inputs.emplace_back(argv[i]);
        graph.lineReader->setInputs(std::move(inputs));
        return graph.repl->run();
    }

    graph::Interactive graph;
    return graph.repl->run();
}
