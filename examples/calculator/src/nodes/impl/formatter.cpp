module examples.calculator.node.formatter;

import examples.calculator.types;
import std;

namespace examples::calculator::node {

namespace {

auto joinParams(std::span<std::string const> params) -> std::string
{
    std::string result;
    for (std::size_t i = 0; i < params.size(); ++i)
    {
        if (i > 0)
            result += ", ";
        result += params[i];
    }
    return result;
}

}

auto Formatter::formatResult(double value) const -> std::string
{
    // Format as integer if the value has no fractional part
    if (value == std::floor(value) && std::isfinite(value))
        return std::format("{:.0f}", value);

    // Otherwise use the default "g" style: significant digits, no trailing zeros
    std::string s = std::format("{:g}", value);
    return s;
}

auto Formatter::formatError(std::string_view message) const -> std::string
{
    return std::format("Error: {}", message);
}

auto Formatter::formatAssignment(std::string_view name, double value) const -> std::string
{
    return std::format("{} = {}", name, formatResult(value));
}

auto Formatter::formatVariables(std::span<std::pair<std::string, double> const> vars) const -> std::string
{
    if (vars.empty())
        return {};

    std::string result;
    bool first = true;
    for (auto const& [name, value] : vars)
    {
        if (!first)
            result += '\n';
        result += std::format("  {} = {}", name, formatResult(value));
        first = false;
    }
    return result;
}

auto Formatter::formatFunctions(std::span<std::string const> builtins, std::span<std::pair<std::string, UserFunction const*> const> userFuncs) const -> std::string
{
    std::string result;
    if (!builtins.empty())
    {
        result += "Built-in: ";
        bool first = true;
        for (auto const& name : builtins)
        {
            if (!first)
                result += ", ";
            first = false;
            result += name;
        }
    }
    for (auto const& [key, ufn] : userFuncs)
    {
        if (!result.empty())
            result += '\n';
        result += std::format("{}({}) = {}", key, joinParams(ufn->params), ufn->source);
    }
    return result;
}

auto Formatter::formatFunctionDef(std::string_view name, std::span<std::string const> params) const -> std::string
{
    return std::format("{}({}) defined", name, joinParams(params));
}

} // namespace examples::calculator::node
