import examples.filesystem.graphs;

auto main(int argc, char* argv[]) -> int
{
    examples::filesystem::graph::DiskRepl graph;
    return graph.repl->run(argc, argv);
}
