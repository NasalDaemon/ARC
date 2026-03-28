export module examples.calculator.formatter;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Formatter : arc::NodeImpl<trait::Formatter>
{
    auto formatResult(double value) const -> std::string;
    auto formatError(std::string_view message) const -> std::string;
    auto formatAssignment(std::string_view name, double value) const -> std::string;
    auto formatVariables(std::span<std::pair<std::string, double> const> vars) const -> std::string;
    auto formatFunctions(std::span<std::string const> names) const -> std::string;
};

} // namespace examples::calculator::node
