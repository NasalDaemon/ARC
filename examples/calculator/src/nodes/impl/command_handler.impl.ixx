module examples.calculator.node.command_handler:impl;

import examples.calculator.node.command_handler;
import std;

#define COMMAND_HANDLER \
    template<class Context> \
    auto CommandHandler::Node<Context>

namespace examples::calculator::node {

COMMAND_HANDLER::isCommand(std::string_view input) const -> bool
{
    static constexpr std::string_view commands[] = {
        "help", "vars", "fns", "history", "clear", "save", "load", "undef"
    };
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
            "  clear    - clear all variables and functions\n"
            "  save [path] - save state to file (default: calculator.state)\n"
            "  load [path] - load state from file (default: calculator.state)\n"
            "  undef f [g ...] - remove user-defined function(s)"
        );
    }
    else if (cmd == "vars")
    {
        return getFormatter().formatVariables(getVariables().list());
    }
    else if (cmd == "fns")
    {
        return getFormatter().formatFunctions(getBuiltinFunctions().list(), getUserFunctions().list());
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
        getUserFunctions().clear();
        return std::string("Variables and user functions cleared.");
    }
    else if (cmd == "save")
    {
        auto path = arg.empty() ? std::string_view{"calculator.state"} : arg;
        auto result = getPersistence().save(path);
        if (!result)
            return std::unexpected(result.error());
        return std::format("Saved to {}", path);
    }
    else if (cmd == "load")
    {
        auto path = arg.empty() ? std::string_view{"calculator.state"} : arg;
        auto result = getPersistence().load(path);
        if (!result)
            return std::unexpected(result.error());
        return std::format("Loaded from {}", path);
    }
    else if (cmd == "undef")
    {
        if (arg.empty())
            return std::unexpected(std::string("Usage: undef <function-name> [...]"));

        std::vector<std::string> names;
        std::string args_str(arg);
        std::istringstream iss(args_str);
        std::string name;
        while (iss >> name)
            names.push_back(name);

        auto result = getUserFunctions().remove(std::span(names));
        if (!result.has_value())
            return std::unexpected(getFormatter().formatError(result.error().message));

        return std::string("Function(s) removed.");
    }

    return std::unexpected(std::format("Unknown command: {}", cmd));
}

} // namespace examples::calculator::node
