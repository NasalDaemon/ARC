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

        // Check for function definition first
        if (std::holds_alternative<FuncDefExpr>(**expr))
        {
            auto const& funcDef = std::get<FuncDefExpr>(**expr);
            getOutput().writeLine(getFormatter().formatFunctionDef(funcDef.name, funcDef.params));
        }
        else if (std::holds_alternative<AssignExpr>(**expr))
        {
            auto const& assign = std::get<AssignExpr>(**expr);
            getVariables().set(std::string{"ans"}, *result);
            getOutput().writeLine(getFormatter().formatAssignment(assign.name, *result));
        }
        else
        {
            getVariables().set(std::string{"ans"}, *result);
            getOutput().writeLine(getFormatter().formatResult(*result));
        }
    }
    return 0;
}

} // namespace examples::calculator::node
