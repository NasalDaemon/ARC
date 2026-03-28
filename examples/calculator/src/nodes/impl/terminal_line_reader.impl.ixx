module;
#include <cstdio>
#include <termios.h>
#include <unistd.h>
module examples.calculator.terminal_line_reader:impl;

import examples.calculator.terminal_line_reader;
import std;

#define TERMINAL_LINE_READER \
    template<class Context> \
    auto TerminalLineReader::Node<Context>

namespace examples::calculator::node {

TERMINAL_LINE_READER::readLine(std::string_view prompt) -> std::optional<std::string>
{
    // Print the prompt
    std::print("{}", prompt);
    std::fflush(stdout);

    // Save current terminal settings
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    // Put terminal in raw mode (disable canonical input and echo)
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    auto restore_terminal = [&]() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    };

    // Get history entries for up/down navigation
    auto history = this->getHistory().entries();

    std::string line;
    std::size_t historyIndex = history.size(); // past-the-end = current (unsaved) line
    std::size_t cursorPos = 0;
    bool gotEof = false;

    // Redraw current line after any change (erases to end of line)
    auto const redraw_line = [&](std::string_view current_line) {
        std::print("\r{}{}\x1b[K", prompt, current_line);
        std::fflush(stdout);
        // Reposition cursor if not at end
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
            // Ctrl+D at empty line = EOF; mid-line = submit
            if (line.empty())
            {
                gotEof = true;
                break;
            }
            // Treat as end-of-line submit when there is content
            break;
        }

        if (ch == '\n' || ch == '\r')
        {
            break;
        }

        // Backspace (127 = DEL sent by most terminals as backspace)
        if (ch == 127 || ch == 8)
        {
            if (cursorPos > 0)
            {
                line.erase(cursorPos - 1, 1);
                --cursorPos;
                redraw_line(line);
            }
            continue;
        }

        // Escape sequence (arrow keys, Delete key)
        if (ch == 27)
        {
            int next = std::getchar();
            if (next == '[')
            {
                int seq = std::getchar();
                if (seq == 'A') // Up arrow — older history entry
                {
                    if (historyIndex > 0)
                    {
                        --historyIndex;
                        line = history[historyIndex];
                        cursorPos = line.size();
                        redraw_line(line);
                    }
                }
                else if (seq == 'B') // Down arrow — newer / blank
                {
                    if (historyIndex < history.size())
                    {
                        ++historyIndex;
                        line = (historyIndex < history.size()) ? history[historyIndex] : std::string{};
                        cursorPos = line.size();
                        redraw_line(line);
                    }
                }
                else if (seq == 'D') // Left arrow
                {
                    if (cursorPos > 0)
                    {
                        --cursorPos;
                        std::print("\x1b[D");
                        std::fflush(stdout);
                    }
                }
                else if (seq == 'C') // Right arrow
                {
                    if (cursorPos < line.size())
                    {
                        ++cursorPos;
                        std::print("\x1b[C");
                        std::fflush(stdout);
                    }
                }
                else if (seq == '3') // Delete key (ESC [ 3 ~)
                {
                    int tilde = std::getchar();
                    if (tilde == '~' && cursorPos < line.size())
                    {
                        line.erase(cursorPos, 1);
                        redraw_line(line);
                    }
                }
            }
            continue;
        }

        // Printable character — insert at cursor position
        if (std::isprint(ch))
        {
            line.insert(cursorPos, 1, static_cast<char>(ch));
            ++cursorPos;
            redraw_line(line);
        }
    }

    restore_terminal();
    std::println();

    if (gotEof)
        return std::nullopt;

    return line;
}

}
