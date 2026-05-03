module;
#include <cstdio>
module examples.filesystem.console_output;

import std;

namespace examples::filesystem::node {

auto ConsoleOutput::write(std::string_view text) -> void
{
    std::print("{}", text);
    std::fflush(stdout);
}

auto ConsoleOutput::writeLine(std::string_view text) -> void
{
    std::println("{}", text);
}

} // namespace examples::filesystem::node
