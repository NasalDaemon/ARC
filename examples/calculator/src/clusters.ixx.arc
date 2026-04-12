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
    repl = node::Repl
    lineReader = Root::LineReader
    output = Root::Output
    tokeniser = node::Tokeniser
    parser = node::Parser
    evaluator = node::Evaluator
    variables = node::Variables
    functions = node::Functions
    formatter = node::Formatter
    history = node::HistoryStore
    commands = node::CommandHandler
    persistence = node::FilePersistence

    // Repl orchestrates the pipeline
    [LineReader]   repl --> lineReader
    [Commands]     repl --> commands
    [Tokeniser]    repl --> tokeniser   <-- persistence
    [Parser]       repl --> parser      <-- persistence
    [Evaluator]    repl --> evaluator
    [Output]       repl --> output
    [History]      repl --> history     <-- commands, lineReader
    [Variables]    repl --> variables   <-- commands, evaluator, persistence, functions
    [Formatter]    repl --> formatter   <-- commands

    [Functions]             functions   <-- commands, evaluator, persistence
    [Persistence]           persistence <-- commands
}

} // namespace examples::calculator
