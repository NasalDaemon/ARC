export module examples.calculator.terminal_line_reader;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct TerminalLineReader
{
    template<class Context>
    struct Node : arc::Node::
        Uses<History>::
        Impl<trait::LineReader>
    {
        auto readLine(std::string_view prompt) -> std::optional<std::string>;
    };
};

}
