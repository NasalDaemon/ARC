module examples.calculator.node.repl:impl;

import examples.calculator.node.repl;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#define REPL \
    template<class Context> \
    auto Repl::Node<Context>

namespace examples::calculator::node {

REPL::run() -> int
{
    while (true)
    {
        auto line = getLineReader().readLine("> ");
        if (!line)
            break;  // EOF

        if (line->empty())
            continue;

        if (*line == "quit" || *line == "exit")
            break;

        getHistory().addEntry(*line);

        // Try command dispatch
        if (getCommands().isCommand(*line))
        {
            auto result = getCommands().execute(*line);
            if (result.has_value())
                getOutput().writeLine(*result);
            else
                getOutput().writeLine(getFormatter().formatError(result.error()));
            continue;
        }

        // Evaluate as expression
        auto tokens = getTokeniser().tokenise(*line);
        if (!tokens)
        {
            getOutput().writeLine(getFormatter().formatError(tokens.error().message));
            continue;
        }

        auto expr = getParser().parse(*tokens, *line);
        if (!expr)
        {
            getOutput().writeLine(getFormatter().formatError(expr.error().message));
            continue;
        }

        auto result = getEvaluator().evaluate(**expr);
        if (!result)
        {
            getOutput().writeLine(getFormatter().formatError(result.error().message));
            continue;
        }

        std::visit([this]<class T>(T const& r) {
            if constexpr (std::is_same_v<T, FuncDefResult>)
            {
                getOutput().writeLine(getFormatter().formatFunctionDef(r.name, r.params));
            }
            else if constexpr (std::is_same_v<T, AssignResult>)
            {
                getOutput().writeLine(getFormatter().formatAssignment(r.name, r.value));
            }
            else
            {
                static_assert(std::is_same_v<T, NumberResult>);
                getOutput().writeLine(getFormatter().formatResult(r.value));
            }
        }, *result);
    }
    return 0;
}

} // namespace examples::calculator::node
