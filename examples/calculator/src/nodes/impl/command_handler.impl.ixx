module examples.calculator.command_handler:impl;

import examples.calculator.command_handler;
import std;

#define COMMAND_HANDLER \
    template<class Context> \
    auto CommandHandler::Node<Context>

namespace examples::calculator::node {

COMMAND_HANDLER::isCommand(std::string_view input) const -> bool
{
    static constexpr std::string_view commands[] = {
        "help", "vars", "fns", "history", "clear", "save", "load"
    };
    // Match the first word of input against known commands
    auto space = input.find(' ');
    auto first_word = input.substr(0, space);
    for (auto cmd : commands) {
        if (first_word == cmd)
            return true;
    }
    return false;
}

COMMAND_HANDLER::execute(std::string_view input) -> std::expected<std::string, std::string>
{
    auto space = input.find(' ');
    auto cmd = input.substr(0, space);
    auto arg = (space != std::string_view::npos) ? input.substr(space + 1) : std::string_view{};

    if (cmd == "help")
    {
        return std::string(
            "Commands:\n"
            "  help     - show this help\n"
            "  vars     - list all variables\n"
            "  fns      - list all functions\n"
            "  history  - show expression history\n"
            "  clear    - clear all variables\n"
            "  save [path] - save state to file (default: calculator.state)\n"
            "  load [path] - load state from file (default: calculator.state)"
        );
    }
    else if (cmd == "vars")
    {
        auto vars = getVariables().list();
        if (vars.empty())
            return std::string("No variables defined.");
        return getFormatter().formatVariables(vars);
    }
    else if (cmd == "fns")
    {
        auto names = getFunctions().list();
        if (names.empty())
            return std::string("No functions defined.");
        return getFormatter().formatFunctions(names);
    }
    else if (cmd == "history")
    {
        auto entries = getHistory().entries();
        if (entries.empty())
            return std::string("No history.");
        std::string result;
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            if (i > 0)
                result += '\n';
            result += std::format("{:3}  {}", i + 1, entries[i]);
        }
        return result;
    }
    else if (cmd == "clear")
    {
        getVariables().clear();
        return std::string("Variables cleared.");
    }
    else if (cmd == "save")
    {
        auto path = arg.empty() ? std::string_view{"calculator.state"} : arg;
        auto result = getPersistence().save(path);
        if (!result)
            return std::unexpected(result.error());
        return std::string("Saved to ") + std::string(path);
    }
    else if (cmd == "load")
    {
        auto path = arg.empty() ? std::string_view{"calculator.state"} : arg;
        auto result = getPersistence().load(path);
        if (!result)
            return std::unexpected(result.error());
        return std::string("Loaded from ") + std::string(path);
    }

    return std::unexpected(std::string("Unknown command: ") + std::string(cmd));
}

} // namespace examples::calculator::node
