import examples.filesystem.graphs;

auto main(int argc, char* argv[]) -> int
{
    examples::filesystem::graph::InMemoryRepl graph;
    return graph.repl->run(argc, argv);
}
