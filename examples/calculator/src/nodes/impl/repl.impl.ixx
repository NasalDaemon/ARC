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
    std::string line;
    while (std::cout << "> ", std::getline(std::cin, line))
    {
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;
        processLine(line);
    }
    return 0;
}

REPL::processLine(std::string_view line) -> void
{
    if (line == "vars")
    {
        auto vars = getVariables().list();
        std::cout << getFormatter().formatVariables(vars) << '\n';
        return;
    }
    if (line == "fns")
    {
        auto names = getFunctions().list();
        std::cout << getFormatter().formatFunctions(names) << '\n';
        return;
    }

    auto tokens = getTokeniser().tokenise(line);
    if (!tokens)
    {
        std::cout << getFormatter().formatError(tokens.error().message) << '\n';
        return;
    }

    auto expr = getParser().parse(*tokens);
    if (!expr)
    {
        std::cout << getFormatter().formatError(expr.error().message) << '\n';
        return;
    }

    auto result = getEvaluator().evaluate(**expr);
    if (!result)
    {
        std::cout << getFormatter().formatError(result.error().message) << '\n';
        return;
    }

    getVariables().set("ans", *result);

    if (std::holds_alternative<AssignExpr>(*(*expr)))
    {
        auto const& assign = std::get<AssignExpr>(*(*expr));
        std::cout << getFormatter().formatAssignment(assign.name, *result) << '\n';
    }
    else
    {
        std::cout << getFormatter().formatResult(*result) << '\n';
    }
}

}
