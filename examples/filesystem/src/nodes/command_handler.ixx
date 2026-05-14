export module examples.filesystem.command_handler;

import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::node {

// CommandHandler dispatches REPL commands to the filesystem and directory sync
export struct CommandHandler
{
    template<class Context>
    struct Node : arc::Node::
        Uses<Filesystem, DirectorySync*>::
        Impl<Commands>
    {
        using Result = std::expected<std::string, std::string>;

        auto isCommand(std::string_view input) const -> bool;
        auto execute(std::span<std::string_view> args) -> Result;

    private:
        auto cmdLs(std::span<std::string_view> args) -> Result;
        auto cmdCat(std::span<std::string_view> args) -> Result;
        auto cmdWrite(std::span<std::string_view> args) -> Result;
        auto cmdMkdir(std::span<std::string_view> args) -> Result;
        auto cmdRm(std::span<std::string_view> args) -> Result;
        auto cmdTree(std::span<std::string_view> args) -> Result;
        auto cmdExists(std::span<std::string_view> args) -> Result;
        auto cmdHelp() -> Result;
        auto buildTree(std::string_view path, std::string_view prefix) -> std::string;
    };
};

} // namespace examples::filesystem::node
