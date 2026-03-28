module examples.calculator.console_output;

import std;

namespace examples::calculator::node {

auto ConsoleOutput::write(std::string_view text) -> void
{
    std::print("{}", text);
}

auto ConsoleOutput::writeLine(std::string_view text) -> void
{
    std::println("{}", text);
}

} // namespace examples::calculator::node
