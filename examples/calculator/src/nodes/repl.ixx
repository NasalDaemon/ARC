export module examples.calculator.repl;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Repl
{
    template<class Context>
    struct Node : arc::NodeUses<Tokeniser, Parser, Evaluator, Variables, Functions, Formatter>
    {
        auto run() -> int;

    private:
        auto processLine(std::string_view line) -> void;
    };
};

}
