module examples.calculator.formatter;

import std;

namespace examples::calculator::node {

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

auto Formatter::formatFunctions(std::span<std::string const> names) const -> std::string
{
    if (names.empty())
        return {};

    std::string result;
    bool first = true;
    for (auto const& name : names)
    {
        if (!first)
            result += ", ";
        result += name;
        first = false;
    }
    return result;
}

} // namespace examples::calculator::node
