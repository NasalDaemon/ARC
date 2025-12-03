module;
#include <cstdio>
#include <termios.h>
#include <unistd.h>
module examples.filesystem.repl:impl;

import examples.filesystem.repl;
import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;


#define REPL \
    template<class Context> \
    auto Repl::Node<Context>

namespace examples::filesystem {

void parseLine(std::string_view line, std::vector<std::string>& argsOut)
{
    argsOut.clear();
    std::string current;

    for (char c : line)
    {
        if (c == ' ' || c == '\t')
        {
            if (!current.empty())
            {
                argsOut.push_back(std::move(current));
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        argsOut.push_back(std::move(current));
}

REPL::run(int argc, char* argv[]) -> int
{
    std::vector<std::string> args;
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
        std::println("In-Memory Filesystem REPL");
        std::println("Commands: ls, cat, write, mkdir, rm, tree, {}help, exit", arc::CanResolve<Node, trait::DirectorySync> ? "load, dump, " : "");
        std::println("Navigation: ↑/↓ history, ←/→ cursor, Backspace/Delete edit, Tab completion");
        std::println("");
    }

    std::vector<std::string> history;
    while (true)
    {
        if (line.empty())
        {
            std::print("> ");
            line = readLineWithHistory(history);
            if (line.empty())
                continue;
        }

        history.push_back(line);

        parseLine(line, args);
        line.clear();
        if (args.empty())
            continue;

        if (not runCommand(args))
            break;
    }

    return 0;
}

REPL::runCommand(std::vector<std::string>& args) -> bool
{
    auto filesystem = getNode(trait::filesystem);

    auto const& cmd = args[0];

    if (cmd == "exit" || cmd == "quit" || cmd == "q")
        return false;

    if constexpr (arc::CanResolve<Node, trait::DirectorySync>)
    {
        auto sync = getNode(trait::directorySync);

        if (cmd == "load")
        {
            if (args.size() < 2)
            {
                std::println("Usage: load <directory_path>");
                return true;
            }
            sync.loadFromDirectory(args[1]);
            return true;
        }

        if (cmd == "dump")
        {
            if (args.size() < 2)
            {
                std::println("Usage: dump <directory_path>");
                return true;
            }
            sync.dumpToDirectory(args[1]);
            return true;
        }
    }

    if (cmd == "help" || cmd == "?")
    {
        std::println("Available commands:");
        std::println("  ls [path]              - List directory contents");
        std::println("  cat <path>             - Display file contents");
        std::println("  write <path> <content> - Write content to file");
        std::println("  mkdir <path>           - Create directory");
        std::println("  rm <path>              - Remove file or directory");
        std::println("  tree [path]            - Display directory tree");
        std::println("  load <dir>             - Load filesystem from directory");
        std::println("  dump <dir>             - Dump filesystem to directory");
        std::println("  exists <path>          - Check if path exists");
        std::println("  help (?)               - Show this help");
        std::println("  exit (quit/q)          - Exit the REPL");
        std::println("");
        std::println("Navigation:");
        std::println("  ↑/↓ arrows             - Navigate command history");
        std::println("  ←/→ arrows             - Move cursor left/right");
        std::println("  Tab                    - Auto-complete file paths");
        return true;
    }

    if (cmd == "ls")
    {
        std::string_view path = args.size() > 1 ? std::string_view(args[1]) : "";
        auto result = filesystem.list(path);
        if (result)
        {
            auto files = std::ranges::stable_partition(*result, [&](std::string_view name) {
                return not filesystem.isDir(std::format("{}/{}", path, name));
            });

            for (auto const& dir : std::ranges::subrange(result->begin(), files.begin()))
                std::println("{}/", dir);

            for (auto const& file : files)
                std::println("{}", file);
        }
        else
        {
            std::println("Error: {}", asString(result.error()));
        }
    }
    else if (cmd == "cat")
    {
        if (args.size() < 2)
        {
            std::println("Usage: cat <path>");
            return true;
        }
        auto result = filesystem.read(args[1]);
        if (result)
            std::println("{}", *result);
        else
            std::println("Error: {}", asString(result.error()));
    }
    else if (cmd == "write")
    {
        if (args.size() < 3)
        {
            std::println("Usage: write <path> <content>");
            return true;
        }
        std::string content(args[2]);
        for (std::size_t i = 3; i < args.size(); ++i)
        {
            content += ' ';
            content += args[i];
        }
        auto result = filesystem.write(args[1], std::move(content));
        if (!result)
            std::println("Error: {}", asString(result.error()));
    }
    else if (cmd == "mkdir")
    {
        if (args.size() < 2)
        {
            std::println("Usage: mkdir <path>");
            return true;
        }
        auto result = filesystem.mkdir(args[1]);
        if (!result)
            std::println("Error: {}", asString(result.error()));
    }
    else if (cmd == "rm")
    {
        if (args.size() < 2)
        {
            std::println("Usage: rm <path>");
            return true;
        }
        auto result = filesystem.remove(args[1]);
        if (!result)
            std::println("Error: {}", asString(result.error()));
    }
    else if (cmd == "tree")
    {
        std::string_view path = args.size() > 1 ? std::string_view(args[1]) : "";
        printTree(path, "");
    }
    else if (cmd == "exists")
    {
        if (args.size() < 2)
        {
            std::println("Usage: exists <path>");
            return true;
        }
        std::println("{}", filesystem.exists(args[1]) ? "true" : "false");
    }
    else
    {
        std::println("Unknown command: {}", cmd);
    }

    return true;
}

REPL::printTree(std::string_view path, std::string_view prefix) -> void
{
    auto filesystem = getNode(trait::filesystem);

    if (path.back() == '/')
        path.remove_suffix(1);

    auto results = filesystem.list(path);
    if (!results)
    {
        std::println("{}Error: {}", prefix, asString(results.error()));
        return;
    }

    auto& children = *results;

    for (std::size_t i = 0; i < children.size(); ++i)
    {
        bool isLast = (i == children.size() - 1);
        auto const& name = children[i];

        std::string childPath = std::format("{}/{}", path, name);
        bool isDir = filesystem.isDir(childPath);

        std::println("{}{}{}{}", prefix, isLast ? "└── " : "├── ", name, isDir ? "/" : "");

        if (isDir)
        {
            printTree(childPath, std::format("{}{}", prefix, (isLast ? "    " : "│   ")));
        }
    }
}

REPL::readLineWithHistory(std::vector<std::string>& history) -> std::string
{
    // Save current terminal settings
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    // Put terminal in raw mode
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    auto restore_terminal = [&]() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    };

    std::string line;
    std::size_t historyIndex = history.size();
    std::size_t cursorPos = 0;

    auto const reposition_cursor = [&](std::string_view current_line) {
        if (cursorPos < current_line.size())
        {
            // Move cursor left by (line.size() - cursorPos) positions
            std::print("\x1b[{}D", current_line.size() - cursorPos);
            std::fflush(stdout);
        }
    };

    auto const redraw_line = [&](std::string_view current_line) {
        std::print("\r> {}\x1b[K", current_line);
        std::fflush(stdout);
        reposition_cursor(current_line);
    };

    while (true)
    {
        int ch = std::getchar();
        if (ch == EOF || ch == '\n' || ch == '\r')
            break;

        if (ch == '\t')
        {
            std::string currentWord;
            std::size_t wordStart = line.find_last_of(" \t", cursorPos - 1);
            if (wordStart == std::string::npos)
                currentWord = line.substr(0, cursorPos);
            else
                currentWord = line.substr(wordStart + 1, cursorPos - wordStart - 1);

            auto completions = getPathCompletions(currentWord);
            if (not completions.empty())
            {
                std::string commonPrefix = completions[0];
                for (std::size_t i = 1; i < completions.size(); ++i)
                {
                    std::size_t j = 0;
                    while (j < commonPrefix.size() && j < completions[i].size() &&
                           commonPrefix[j] == completions[i][j])
                        ++j;
                    commonPrefix = commonPrefix.substr(0, j);
                    if (commonPrefix.empty())
                        break;
                }

                if (commonPrefix.size() > currentWord.size())
                {
                    if (wordStart == std::string::npos)
                    {
                        line = commonPrefix;
                        cursorPos = commonPrefix.size();
                    }
                    else
                    {
                        line.replace(wordStart + 1, currentWord.size(), commonPrefix);
                        cursorPos = wordStart + 1 + commonPrefix.size();
                    }

                    redraw_line(line);
                }
                else if (completions.size() > 1)
                {
                    std::println("");

                    std::ranges::stable_partition(completions, [](auto const& s) { return s.ends_with('/'); });

                    std::size_t maxToShow = std::min(std::size_t{5}, completions.size());
                    for (std::size_t i = 0; i < maxToShow; ++i)
                    {
                        std::string_view completion = completions[i];
                        bool isDir = completion.ends_with('/');

                        if (isDir)
                            completion.remove_suffix(1);

                        std::size_t lastSlash = completion.rfind('/');
                        std::string_view displayName = (lastSlash != std::string::npos)
                            ? completion.substr(lastSlash + 1)
                            : completion;

                        std::print("{}{}  ", displayName, isDir ? "/" : "");
                    }
                    if (completions.size() > 5)
                    {
                        std::print("...");
                    }
                    std::println("");

                    redraw_line(line);
                }
            }
        }
        else if (ch == 127 || ch == 8)
        {
            if (cursorPos > 0)
            {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                redraw_line(line);
            }
        }
        else if (ch == 27)
        {
            if (std::getchar() == '[')
            {
                int seq = std::getchar();
                if (seq == 'A' && historyIndex > 0)
                {
                    historyIndex--;
                    line = history[historyIndex];
                    cursorPos = line.size();
                    redraw_line(line);
                }
                else if (seq == 'B' && historyIndex < history.size())
                {
                    historyIndex++;
                    if (historyIndex == history.size())
                        line.clear();
                    else
                        line = history[historyIndex];
                    cursorPos = line.size();
                    redraw_line(line);
                }
                else if (seq == 'D' && cursorPos > 0)
                {
                    cursorPos--;
                    std::print("\x1b[D");
                    std::fflush(stdout);
                }
                else if (seq == 'C' && cursorPos < line.size())
                {
                    cursorPos++;
                    std::print("\x1b[C");
                    std::fflush(stdout);
                }
                else if (seq == '3')
                {
                    // DEL key - delete character at cursor position
                    if (std::getchar() == '~' && cursorPos < line.size())
                    {
                        line.erase(cursorPos, 1);
                        redraw_line(line);
                    }
                }
            }
        }
        else if (std::isprint(ch))
        {
            line.insert(cursorPos, 1, static_cast<char>(ch));
            cursorPos++;
            redraw_line(line);
        }
    }

    restore_terminal();
    std::println();
    return line;
}

REPL::getPathCompletions(std::string_view prefix) -> std::vector<std::string>
{
    auto filesystem = getNode(trait::filesystem);

    std::vector<std::string> completions;
    std::string_view prefixStr{prefix};

    std::size_t lastSlash = prefixStr.rfind('/');
    std::string_view dirPath;
    std::string_view filePrefix;

    if (lastSlash == std::string::npos)
    {
        dirPath = "/";
        filePrefix = prefixStr;
    }
    else
    {
        dirPath = prefixStr.substr(0, lastSlash + 1);
        if (dirPath.empty()) dirPath = "/";
        filePrefix = (lastSlash + 1 < prefixStr.size()) ? prefixStr.substr(lastSlash + 1) : "";
    }

    auto result = filesystem.list(dirPath);
    if (result)
    {
        for (auto const& item : *result)
        {
            if (item.starts_with(filePrefix))
            {
                std::string completion = std::format("{}{}", dirPath, item);

                if (filesystem.isDir(completion))
                    completion += '/';

                completions.push_back(std::move(completion));
            }
        }
    }

    return completions;
}

}
