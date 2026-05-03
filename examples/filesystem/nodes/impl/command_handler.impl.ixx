module examples.filesystem.command_handler:impl;

import examples.filesystem.command_handler;
import examples.filesystem.traits;
import arc;
import std;

#define COMMAND_HANDLER \
    template<class Context> \
    auto CommandHandler::Node<Context>

namespace examples::filesystem::node {

std::string_view const knownCommands[] = {
    "ls", "cat", "write", "mkdir", "rm", "tree", "exists", "load", "dump", "help", "?",
};

COMMAND_HANDLER::isCommand(std::string_view input) const -> bool
{
    // Extract first word
    auto const pos = input.find_first_of(" \t");
    auto const cmd = (pos == std::string_view::npos) ? input : input.substr(0, pos);

    return std::ranges::find(knownCommands, cmd) != std::end(knownCommands);
}

COMMAND_HANDLER::execute(std::span<std::string_view> args) -> Result
{
    auto const& cmd = args[0];

    if (cmd == "help" || cmd == "?")
        return cmdHelp();
    if (cmd == "ls")
        return cmdLs(args);
    if (cmd == "cat")
        return cmdCat(args);
    if (cmd == "write")
        return cmdWrite(args);
    if (cmd == "mkdir")
        return cmdMkdir(args);
    if (cmd == "rm")
        return cmdRm(args);
    if (cmd == "tree")
        return cmdTree(args);
    if (cmd == "exists")
        return cmdExists(args);

    if constexpr (arc::CanGetNode<Node, DirectorySync>)
    {
        auto sync = getNode(directorySync);

        if (cmd == "load")
        {
            if (args.size() < 2)
                return std::unexpected(std::string{"Usage: load <directory_path>"});
            auto result = sync.loadFromDirectory(args[1]);
            if (!result)
                return std::unexpected(std::format("Error: {}", asString(result.error())));
            return std::string{};
        }

        if (cmd == "dump")
        {
            if (args.size() < 2)
                return std::unexpected(std::string{"Usage: dump <directory_path>"});
            auto result = sync.dumpToDirectory(args[1]);
            if (!result)
                return std::unexpected(std::format("Error: {}", asString(result.error())));
            return std::string{};
        }
    }

    return std::unexpected(std::format("Unknown command: {}", cmd));
}

COMMAND_HANDLER::cmdLs(std::span<std::string_view> args) -> Result
{
    auto filesystem = getFilesystem();
    std::string_view path = args.size() > 1 ? args[1] : "/";

    auto result = filesystem.list(path);
    if (!result)
        return std::unexpected(std::format("Error: {}", asString(result.error())));

    std::string output;
    auto files = std::ranges::stable_partition(*result, [&](std::string_view name) {
        std::string childPath = (path == "/") ? std::format("/{}", name) : std::format("{}/{}", path, name);
        return filesystem.isDir(childPath);
    });

    for (auto const& dir : std::ranges::subrange(result->begin(), files.begin()))
        output += std::format("{}/\n", dir);

    for (auto const& file : files)
        output += std::format("{}\n", file);

    return output;
}

COMMAND_HANDLER::cmdCat(std::span<std::string_view> args) -> Result
{
    if (args.size() < 2)
        return std::unexpected(std::string{"Usage: cat <path>"});

    auto result = getFilesystem().read(args[1]);
    if (!result)
        return std::unexpected(std::format("Error: {}", asString(result.error())));
    return std::string{*result};
}

COMMAND_HANDLER::cmdWrite(std::span<std::string_view> args) -> Result
{
    if (args.size() < 3)
        return std::unexpected(std::string{"Usage: write <path> <content>"});

    std::string content(args[2]);
    for (std::size_t i = 3; i < args.size(); ++i)
    {
        content += ' ';
        content += args[i];
    }
    auto result = getFilesystem().write(args[1], std::move(content));
    if (!result)
        return std::unexpected(std::format("Error: {}", asString(result.error())));
    return std::string{};
}

COMMAND_HANDLER::cmdMkdir(std::span<std::string_view> args) -> Result
{
    if (args.size() < 2)
        return std::unexpected(std::string{"Usage: mkdir <path>"});

    auto result = getFilesystem().mkdir(args[1]);
    if (!result)
        return std::unexpected(std::format("Error: {}", asString(result.error())));
    return std::string{};
}

COMMAND_HANDLER::cmdRm(std::span<std::string_view> args) -> Result
{
    if (args.size() < 2)
        return std::unexpected(std::string{"Usage: rm <path>"});

    auto result = getFilesystem().remove(args[1]);
    if (!result)
        return std::unexpected(std::format("Error: {}", asString(result.error())));
    return std::string{};
}

COMMAND_HANDLER::cmdTree(std::span<std::string_view> args) -> Result
{
    std::string_view path = args.size() > 1 ? args[1] : "/";

    if (path.ends_with('/'))
        path.remove_suffix(1);
    if (path.empty())
        path = "/";

    return buildTree(path, "");
}

COMMAND_HANDLER::cmdExists(std::span<std::string_view> args) -> Result
{
    if (args.size() < 2)
        return std::unexpected(std::string{"Usage: exists <path>"});

    return getFilesystem().exists(args[1]) ? std::string{"true"} : std::string{"false"};
}

COMMAND_HANDLER::cmdHelp() -> Result
{
    std::string output;
    output += "Available commands:\n";
    output += "  ls [path]              - List directory contents\n";
    output += "  cat <path>             - Display file contents\n";
    output += "  write <path> <content> - Write content to file\n";
    output += "  mkdir <path>           - Create directory\n";
    output += "  rm <path>              - Remove file or directory\n";
    output += "  tree [path]            - Display directory tree\n";
    output += "  exists <path>          - Check if path exists\n";

    if constexpr (arc::CanGetNode<Node, DirectorySync>)
    {
        output += "  load <dir>             - Load filesystem from directory\n";
        output += "  dump <dir>             - Dump filesystem to directory\n";
    }

    output += "  help (?)               - Show this help\n";
    output += "  exit (quit/q)          - Exit the REPL\n";
    output += "\n";
    output += "Navigation:\n";
    output += "  ↑/↓ arrows             - Navigate command history\n";
    output += "  ←/→ arrows             - Move cursor left/right\n";
    return output;
}

COMMAND_HANDLER::buildTree(std::string_view path, std::string_view prefix) -> std::string
{
    auto filesystem = getFilesystem();

    auto results = filesystem.list(path);
    if (!results)
        return std::format("{}Error: {}\n", prefix, asString(results.error()));

    std::string output;
    auto& children = *results;

    for (std::size_t i = 0; i < children.size(); ++i)
    {
        bool isLast = (i == children.size() - 1);
        auto const& name = children[i];

        std::string childPath = std::format("{}/{}", path, name);
        bool isDir = filesystem.isDir(childPath);

        output += std::format("{}{}{}{}\n", prefix, isLast ? "└── " : "├── ", name, isDir ? "/" : "");

        if (isDir)
            output += buildTree(childPath, std::format("{}{}", prefix, isLast ? "    " : "│   "));
    }

    return output;
}

} // namespace examples::filesystem::node
