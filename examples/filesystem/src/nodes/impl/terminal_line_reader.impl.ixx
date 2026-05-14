module;
#include <cstdio>
#include <termios.h>
#include <unistd.h>
module examples.filesystem.terminal_line_reader:impl;

import examples.filesystem.terminal_line_reader;
import examples.filesystem.traits;
import arc;
import std;

#define TERMINAL_LINE_READER \
    template<class Context> \
    auto TerminalLineReader::Node<Context>

namespace examples::filesystem::node {

TERMINAL_LINE_READER::getPathCompletions(std::string_view prefix) -> std::vector<std::string>
{
    auto filesystem = getFilesystem();

    std::size_t lastSlash = prefix.rfind('/');
    std::string_view dirPath;
    std::string_view filePrefix;

    if (lastSlash == std::string_view::npos)
    {
        dirPath = "/";
        filePrefix = prefix;
    }
    else
    {
        dirPath = prefix.substr(0, lastSlash + 1);
        if (dirPath.empty()) dirPath = "/";
        filePrefix = (lastSlash + 1 < prefix.size()) ? prefix.substr(lastSlash + 1) : "";
    }

    // Strip trailing slash for filesystem.list() which expects normalised paths
    std::string_view listPath = dirPath;
    if (listPath.ends_with('/') && listPath.size() > 1)
        listPath.remove_suffix(1);

    auto result = filesystem.list(listPath);
    if (!result)
        return {};

    std::vector<std::string> completions;
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

    return completions;
}

TERMINAL_LINE_READER::readLine(std::string_view prompt) -> std::optional<std::string>
{
    std::print("{}", prompt);

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
    std::size_t cursorPos = 0;

    auto const redraw_line = [&](std::string_view current_line) {
        std::print("\r> {}\x1b[K", current_line);
        std::fflush(stdout);
        if (cursorPos < current_line.size())
        {
            std::print("\x1b[{}D", current_line.size() - cursorPos);
            std::fflush(stdout);
        }
    };

    while (true)
    {
        int ch = std::getchar();
        if (ch == EOF)
        {
            restore_terminal();
            return std::nullopt;
        }
        if (ch == '\n' || ch == '\r')
            break;

        if (ch == '\t') // Tab completion
        {
            // Find the word at cursor position
            std::string currentWord;
            std::size_t wordStart = (cursorPos > 0) ? line.find_last_of(" \t", cursorPos - 1) : std::string::npos;
            if (wordStart == std::string::npos)
            {
                currentWord = line.substr(0, cursorPos);
                wordStart = 0;
            }
            else
            {
                currentWord = line.substr(wordStart + 1, cursorPos - wordStart - 1);
                wordStart++;
            }

            auto completions = getPathCompletions(currentWord);
            if (not completions.empty())
            {
                // Find common prefix among all completions
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
                    // Single or common prefix completion — replace word at cursor
                    line.replace(wordStart, currentWord.size(), commonPrefix);
                    cursorPos = wordStart + commonPrefix.size();
                    redraw_line(line);
                }
                else if (completions.size() > 1)
                {
                    // Multiple completions — show display names
                    std::println("");

                    // Sort directories first
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
                        std::print("...");
                    std::println("");

                    redraw_line(line);
                }
            }
        }
        else if (ch == 127 || ch == 8) // Backspace
        {
            if (cursorPos > 0)
            {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                redraw_line(line);
            }
        }
        else if (ch == 27) // Escape sequence
        {
            if (std::getchar() == '[')
            {
                int seq = std::getchar();
                if (seq == 'A' && historyIndex > 0) // Up
                {
                    historyIndex--;
                    line = history[historyIndex];
                    cursorPos = line.size();
                    redraw_line(line);
                }
                else if (seq == 'B' && historyIndex < history.size()) // Down
                {
                    historyIndex++;
                    if (historyIndex == history.size())
                        line.clear();
                    else
                        line = history[historyIndex];
                    cursorPos = line.size();
                    redraw_line(line);
                }
                else if (seq == 'D' && cursorPos > 0) // Left
                {
                    cursorPos--;
                    std::print("\x1b[D");
                    std::fflush(stdout);
                }
                else if (seq == 'C' && cursorPos < line.size()) // Right
                {
                    cursorPos++;
                    std::print("\x1b[C");
                    std::fflush(stdout);
                }
                else if (seq == '3') // Delete
                {
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
    history.push_back(line);
    historyIndex = history.size();
    return line;
}

} // namespace examples::filesystem::node
