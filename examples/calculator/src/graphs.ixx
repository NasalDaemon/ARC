export module examples.calculator.graphs;

import examples.calculator.clusters;
import examples.calculator.terminal_line_reader;
import examples.calculator.batch_line_reader;
import examples.calculator.console_output;
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

}
