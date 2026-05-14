export module examples.filesystem.repl;

import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// Repl reads input via LineReader, dispatches to Commands, writes via Output
export struct Repl
{
    template<class Context>
    struct Node : arc::NodeUses<LineReader, Commands, Output>
    {
        auto run(int argc, char* argv[]) -> int;

    private:
        auto parseLine(std::string_view line, std::vector<std::string_view>& argsOut) -> void;
    };
};

} // namespace examples::filesystem::node
