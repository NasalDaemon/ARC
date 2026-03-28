export module examples.calculator.console_output;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct ConsoleOutput : arc::NodeImpl<Output>
{
    auto write(std::string_view text) -> void;
    auto writeLine(std::string_view text) -> void;
};

}
