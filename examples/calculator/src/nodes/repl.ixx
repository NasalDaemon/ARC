export module examples.calculator.repl;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Repl
{
    template<class Context>
    struct Node : arc::NodeUses<LineReader, Output, Commands, Tokeniser, Parser, Evaluator, Variables, Formatter, History>
    {
        auto run() -> int;
    };
};

} // namespace examples::calculator::node
