export module examples.filesystem.terminal_line_reader;

import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// TerminalLineReader reads input from the terminal with history, cursor, and tab completion
export struct TerminalLineReader
{
    template<class Context>
    struct Node : arc::Node::
        Uses<Filesystem>::
        Impl<LineReader>
    {
        auto readLine(std::string_view prompt) -> std::optional<std::string>;
        auto getPathCompletions(std::string_view prefix) -> std::vector<std::string>;

    private:
        std::vector<std::string> history;
        std::size_t historyIndex = 0;
    };
};

} // namespace examples::filesystem::node
