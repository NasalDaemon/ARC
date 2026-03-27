export module examples.calculator.clusters;

import examples.calculator.tokeniser;
import examples.calculator.parser;
import examples.calculator.evaluator;
import examples.calculator.variables;
import examples.calculator.functions;
import examples.calculator.formatter;
import examples.calculator.repl;
import examples.calculator.traits;

namespace examples::calculator {

domain Calculator
{
    repl = node::Repl
    tokeniser = node::Tokeniser
    parser = node::Parser
    evaluator = node::Evaluator
    Variables = node::Variables
    functions = node::Functions
    formatter = node::Formatter

    [Tokeniser]  repl --> tokeniser
    [Parser]     repl --> parser
    [Evaluator]  repl --> evaluator
    [Formatter]  repl --> formatter
    [Functions]  repl --> functions
    [Variables]  repl --> Variables

    [Functions]  evaluator --->>> functions
    [Variables]  evaluator --->>> Variables
}

}
