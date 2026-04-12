export module examples.calculator.node.formatter;

import examples.calculator.types;
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
    auto formatFunctions(std::span<std::string const> builtins, std::span<std::pair<std::string, UserFunction const*> const> userFuncs) const -> std::string;
    auto formatFunctionDef(std::string_view name, std::span<std::string const> params) const -> std::string;
};

} // namespace examples::calculator::node
