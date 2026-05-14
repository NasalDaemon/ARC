module examples.filesystem.repl:impl;

import examples.filesystem.repl;
import examples.filesystem.traits;
import arc;
import std;

#define REPL \
    template<class Context> \
    auto Repl::Node<Context>

namespace examples::filesystem::node {

REPL::parseLine(std::string_view line, std::vector<std::string_view>& argsOut) -> void
{
    argsOut.clear();

    std::string_view remaining = line;
    while (!remaining.empty()) {
        std::size_t const pos = remaining.find_first_of(" \t");
        if (pos == std::string_view::npos)
        {
            if (!remaining.empty())
                argsOut.push_back(remaining);
            break;
        }
        else
        {
            if (pos > 0)
                argsOut.push_back(remaining.substr(0, pos));

            std::size_t const nextPos = remaining.find_first_not_of(" \t", pos);
            if (nextPos == std::string_view::npos)
                break;
            remaining.remove_prefix(nextPos);
        }
    }
}

REPL::run(int argc, char* argv[]) -> int
{
    auto lineReader = getLineReader();
    auto commands = getCommands();
    auto output = getOutput();

    std::vector<std::string_view> args;
    std::string line;
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (i > 1) line += ' ';
            line += argv[i];
        }
    }
    else
    {
        output.writeLine("In-Memory Filesystem REPL");
        output.writeLine("Commands: ls, cat, write, mkdir, rm, tree, exists, load, dump, help, exit");
        output.writeLine("Navigation: ↑/↓ history, ←/→ cursor, Backspace/Delete edit");
        output.writeLine("");
    }

    for (;; line.clear())
    {
        if (line.empty())
        {
            auto input = lineReader.readLine("> ");
            if (!input)
                break;
            line = std::move(*input);
            if (line.empty())
                continue;
        }

        parseLine(line, args);
        if (args.empty())
            continue;

        // Handle exit commands directly
        if (args[0] == "exit" || args[0] == "quit" || args[0] == "q")
            break;

        // Dispatch to CommandHandler
        if (commands.isCommand(line))
        {
            auto result = commands.execute(args);
            if (result)
            {
                if (!result->empty())
                    output.writeLine(*result);
            }
            else
            {
                output.writeLine(std::format("Error: {}", result.error()));
            }
        }
        else
        {
            output.writeLine(std::format("Unknown command: {}", args[0]));
        }
    }

    return 0;
}

} // namespace examples::filesystem::node
