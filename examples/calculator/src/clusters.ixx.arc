export module examples.calculator.clusters;

import examples.calculator.node.tokeniser;
import examples.calculator.node.parser;
import examples.calculator.node.evaluator;
import examples.calculator.node.variables;
import examples.calculator.node.functions;
import examples.calculator.node.formatter;
import examples.calculator.node.repl;
import examples.calculator.node.history_store;
import examples.calculator.node.command_handler;
import examples.calculator.node.file_persistence;
import examples.calculator.traits;

namespace examples::calculator {

cluster Calculator [Root]
{
    repl       = node::Repl
    lineReader = Root::LineReader
    output     = Root::Output
    data       = cluster::Data
    compute    = cluster::Compute

    [LineReader]  repl --> lineReader
    [Output]      repl --> output
    [History]     lineReader --> data

    [[cluster::Data]]    repl ==> data
    [[cluster::Compute]] repl, data ==> compute
}

cluster Compute [Trunk = Tokeniser + Parser + Evaluator + Functions + Variables]
{
    tokeniser = node::Tokeniser
    parser    = node::Parser
    evaluator = node::Evaluator
    functions = node::Functions
    variables = node::Variables

    [Tokeniser] .. --> tokeniser
    [Parser]    .. --> parser
    [Evaluator] .. --> evaluator
    [Functions] .. --> functions <-- evaluator
    [Variables] .. --> variables <-- evaluator, functions
}

cluster Data [Trunk = Commands + History + Formatter]
{
    commands    = node::CommandHandler
    formatter   = node::Formatter
    history     = node::HistoryStore
    persistence = node::FilePersistence

    [Commands]    .. --> commands
    [History]     .. --> history     <-- commands
    [Formatter]   .. --> formatter   <-- commands
    [Persistence] .. --> persistence <-- commands

    [[Functions + Variables]]   commands, persistence ==> ..
    [[Tokeniser + Parser]]                persistence ==> ..
}

} // namespace examples::calculator
