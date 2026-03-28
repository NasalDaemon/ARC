export module examples.calculator.command_handler;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct CommandHandler
{
    template<class Context>
    struct Node : arc::Node::
        Impl<Commands>::
        Uses<Variables, Functions, History, Persistence, Formatter>
    {
        auto isCommand(std::string_view input) const -> bool;
        auto execute(std::string_view input) -> std::expected<std::string, std::string>;
    };
};

} // namespace examples::calculator::node
