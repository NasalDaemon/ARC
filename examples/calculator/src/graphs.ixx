export module examples.calculator.graphs;

import examples.calculator.clusters;
import examples.calculator.node.terminal_line_reader;
import examples.calculator.node.batch_line_reader;
import examples.calculator.node.console_output;
import arc;

namespace examples::calculator::graph {

struct InteractiveRoot
{
    using LineReader = node::TerminalLineReader;
    using Output = node::ConsoleOutput;
};
export using Interactive = arc::Graph<cluster::Calculator, InteractiveRoot>;

struct BatchRoot
{
    using LineReader = node::BatchLineReader;
    using Output = node::ConsoleOutput;
};
export using Batch = arc::Graph<cluster::Calculator, BatchRoot>;

} // namespace examples::calculator::graph
