#include <cstdio>
#include <termios.h>
#include <unistd.h>

import examples.filesystem.domain;
import examples.filesystem.entry;
import examples.filesystem.graphs;
import examples.filesystem.traits;
import arc;
import std;

using namespace examples::filesystem;

void parseLine(std::string_view line, std::vector<std::string>& args);
void printTree(auto filesystem, std::string_view path, std::string const& prefix);
auto readLineWithHistory(std::vector<std::string>& history, auto filesystem) -> std::string;
auto getPathCompletions(std::string_view prefix, auto filesystem) -> std::vector<std::string>;

auto main(int argc, char* argv[]) -> int
{
    InMemoryGraph graph;
    auto filesystem = graph.asTrait(trait::filesystem);

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
        // REPL implementation
        std::println("In-Memory Filesystem REPL");
        std::println("Commands: ls, cat, write, mkdir, rm, tree, load, dump, help, exit");
        std::println("");
    }

    std::vector<std::string> history;
    while (true)
    {
        if (line.empty())
        {
            std::print("> ");
            line = readLineWithHistory(history, filesystem);
            if (line.empty())
                continue;
        }

        history.push_back(line);

        parseLine(line, args);
        line.clear();
        if (args.empty())
            continue;

        auto const& cmd = args[0];

        if (cmd == "exit" || cmd == "quit")
            break;

        if (cmd == "load")
        {
            if (args.size() < 2)
            {
                std::println("Usage: load <directory_path>");
                continue;
            }
            graph.Storage->loadFromDirectory(args[1]);
            continue;
        }

        if (cmd == "dump")
        {
            if (args.size() < 2)
            {
                std::println("Usage: dump <directory_path>");
                continue;
            }
            graph.Storage->dumpToDirectory(args[1]);
            continue;
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
            std::println("  exists <path>          - Check if path exists");;
            std::println("  help (?)               - Show this help");
            std::println("  exit (quit)            - Exit the REPL");
            std::println("");
            std::println("Navigation:");
            std::println("  ↑/↓ arrows             - Navigate command history");
            std::println("  Tab                    - Auto-complete file paths");
            continue;
        }

        if (cmd == "ls")
        {
            std::string path = args.size() > 1 ? args[1] : "/";
            auto result = filesystem.list(path);
            if (result)
            {
                for (auto const& n : *result)
                {
                    std::string name(n);
                    bool isDir = filesystem.isDir(path == "/" ? "/" + name : path + "/" + name);
                    std::println("{}{}", name, isDir ? "/" : "");
                }
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
                continue;
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
                continue;
            }
            // Join remaining args as content
            std::string content;
            for (std::size_t i = 2; i < args.size(); ++i)
            {
                if (i > 2) content += ' ';
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
                continue;
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
                continue;
            }
            auto result = filesystem.remove(args[1]);
            if (!result)
                std::println("Error: {}", asString(result.error()));
        }
        else if (cmd == "tree")
        {
            std::string path = args.size() > 1 ? args[1] : "/";
            printTree(filesystem, path, "");
        }
        else if (cmd == "exists")
        {
            if (args.size() < 2)
            {
                std::println("Usage: exists <path>");
                continue;
            }
            std::println("{}", filesystem.exists(args[1]) ? "true" : "false");
        }
        else
        {
            std::println("Unknown command: {}", cmd);
        }
    }

    return 0;
}

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
                current.clear();
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

auto printTree(auto filesystem, std::string_view path, std::string const& prefix) -> void
{
    auto result = filesystem.list(path);
    if (!result)
    {
        std::println("{}Error: {}", prefix, asString(result.error()));
        return;
    }

    auto const& children = *result;
    for (std::size_t i = 0; i < children.size(); ++i)
    {
        bool isLast = (i == children.size() - 1);
        auto const& n = children[i];
        std::string name(n);

        std::string childPath = path == "/" ? "/" + name : std::string{path} + "/" + name;
        bool isDir = filesystem.isDir(childPath);

        std::println("{}{}{}{}", prefix, isLast ? "└── " : "├── ", name, isDir ? "/" : "");

        if (isDir)
        {
            std::string newPrefix = prefix + (isLast ? "    " : "│   ");
            printTree(filesystem, childPath, newPrefix);
        }
    }
}

auto readLineWithHistory(std::vector<std::string>& history, auto filesystem) -> std::string
{
    // Save current terminal settings
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    // Put terminal in raw mode
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    raw.c_cc[VMIN] = 1;              // Read at least 1 character
    raw.c_cc[VTIME] = 0;             // No timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    // Ensure terminal is restored on exit
    auto restore_terminal = [&]() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    };

    std::string line;
    std::size_t historyIndex = history.size();
    std::size_t cursorPos = 0;

    while (true)
    {
        int ch = std::getchar();
        if (ch == EOF || ch == '\n' || ch == '\r')
            break;

        if (ch == '\t')
        {
            // Tab completion for file paths - complete the current word
            std::string currentWord;
            std::size_t wordStart = line.find_last_of(" \t", cursorPos - 1);
            if (wordStart == std::string::npos)
                currentWord = line.substr(0, cursorPos);
            else
                currentWord = line.substr(wordStart + 1, cursorPos - wordStart - 1);

            auto completions = getPathCompletions(currentWord, filesystem);
            if (not completions.empty())
            {
                // Find the common prefix of all completions
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

                // If common prefix is longer than current word, complete to it
                if (commonPrefix.size() > currentWord.size())
                {
                    // Replace the current word with the common prefix
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

                    std::print("\r> {}\x1b[K", line);
                    std::fflush(stdout);
                }
                else if (completions.size() > 1)
                {
                    // Multiple completions with no further common prefix - show them
                    std::println(""); // Move to next line

                    // Separate directories and files, show directories first
                    std::vector<std::string> dirs;
                    std::vector<std::string> files;

                    for (auto const& completion : completions)
                    {
                        if (completion.ends_with('/'))
                            dirs.push_back(completion);
                        else
                            files.push_back(completion);
                    }

                    // Combine directories first, then files
                    std::vector<std::string> sortedCompletions;
                    sortedCompletions.insert(sortedCompletions.end(), dirs.begin(), dirs.end());
                    sortedCompletions.insert(sortedCompletions.end(), files.begin(), files.end());

                    std::size_t maxToShow = std::min(std::size_t{5}, sortedCompletions.size());
                    for (std::size_t i = 0; i < maxToShow; ++i)
                    {
                        // Show only the filename component, not the full path
                        std::string completion = sortedCompletions[i];
                        bool isDir = completion.ends_with('/');

                        // Remove trailing slash for directories before extracting filename
                        if (isDir)
                            completion = completion.substr(0, completion.size() - 1);

                        std::size_t lastSlash = completion.rfind('/');
                        std::string displayName = (lastSlash != std::string::npos)
                            ? completion.substr(lastSlash + 1)
                            : completion;

                        // Add trailing slash back for directories in display
                        if (isDir)
                            displayName += '/';

                        std::print("{}  ", displayName);
                    }
                    if (sortedCompletions.size() > 5)
                    {
                        std::print("...");
                    }
                    std::println(""); // New line

                    // Redisplay the prompt and current line
                    std::print("> {}\x1b[K", line);
                    std::fflush(stdout);
                }
                // If there's exactly one completion and it's already fully entered, do nothing
            }
        }
        else if (ch == 127 || ch == 8) // Backspace
        {
            if (cursorPos > 0)
            {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                std::print("\r> {}\x1b[K", line);
                std::fflush(stdout);
            }
        }
        else if (ch == 27) // Escape sequence start
        {
            // Handle arrow keys and other escape sequences
            if (std::getchar() == '[') // CSI sequence
            {
                int seq = std::getchar();
                if (seq == 'A' && historyIndex > 0) // Up arrow
                {
                    historyIndex--;
                    line = history[historyIndex];
                    cursorPos = line.size();
                    std::print("\r> {}\x1b[K", line);
                    std::fflush(stdout);
                }
                else if (seq == 'B' && historyIndex < history.size()) // Down arrow
                {
                    historyIndex++;
                    if (historyIndex == history.size())
                        line.clear();
                    else
                        line = history[historyIndex];
                    cursorPos = line.size();
                    std::print("\r> {}\x1b[K", line);
                    std::fflush(stdout);
                }
            }
        }
        else if (std::isprint(ch)) // Printable character
        {
            line.insert(cursorPos, 1, static_cast<char>(ch));
            cursorPos++;
            std::print("\r> {}\x1b[K", line);
            std::fflush(stdout);
        }
    }

    restore_terminal();
    std::println("");
    return line;
}

auto getPathCompletions(std::string_view prefix, auto filesystem) -> std::vector<std::string>
{
    std::vector<std::string> completions;

    // Simple path completion - find the directory part and list its contents
    std::string prefixStr{prefix};

    // Find the last path separator
    std::size_t lastSlash = prefixStr.rfind('/');
    std::string dirPath;
    std::string filePrefix;

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

    // List directory contents
    auto result = filesystem.list(dirPath);
    if (result)
    {
        for (auto const& item : *result)
        {
            std::string itemStr{item};
            if (itemStr.starts_with(filePrefix))
            {
                std::string completion;
                if (dirPath == "/")
                    completion = "/" + itemStr;
                else
                    completion = dirPath + itemStr;

                // Check if it's a directory and add trailing slash
                if (filesystem.isDir(completion))
                    completion += '/';

                completions.push_back(completion);
            }
        }
    }

    return completions;
}
