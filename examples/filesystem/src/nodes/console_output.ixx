export module examples.filesystem.console_output;

import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// ConsoleOutput writes to stdout
export struct ConsoleOutput : arc::NodeImpl<Output>
{
    auto write(std::string_view text) -> void;
    auto writeLine(std::string_view text) -> void;
};

} // namespace examples::filesystem::node
