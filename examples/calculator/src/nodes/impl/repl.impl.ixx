module examples.calculator.repl:impl;

import examples.calculator.repl;
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

        // Try command dispatch
        if (getCommands().isCommand(*line))
        {
            auto result = getCommands().execute(*line);
            if (result.has_value())
                getOutput().writeLine(*result);
            else
                getOutput().writeLine(getFormatter().formatError(result.error()));
            getHistory().addEntry(*line);
            continue;
        }

        // Evaluate as expression
        auto tokens = getTokeniser().tokenise(*line);
        if (!tokens)
        {
            getOutput().writeLine(getFormatter().formatError(tokens.error().message));
            getHistory().addEntry(*line);
            continue;
        }

        auto expr = getParser().parse(*tokens);
        if (!expr)
        {
            getOutput().writeLine(getFormatter().formatError(expr.error().message));
            getHistory().addEntry(*line);
            continue;
        }

        auto result = getEvaluator().evaluate(**expr);
        if (!result)
        {
            getOutput().writeLine(getFormatter().formatError(result.error().message));
            getHistory().addEntry(*line);
            continue;
        }

        // Set ans and format output
        getVariables().set(std::string{"ans"}, *result);

        if (std::holds_alternative<AssignExpr>(**expr))
        {
            auto const& assign = std::get<AssignExpr>(**expr);
            getOutput().writeLine(getFormatter().formatAssignment(assign.name, *result));
        }
        else
        {
            getOutput().writeLine(getFormatter().formatResult(*result));
        }

        getHistory().addEntry(*line);
    }
    return 0;
}

} // namespace examples::calculator::node
