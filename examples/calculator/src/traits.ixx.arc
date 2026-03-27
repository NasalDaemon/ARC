export module examples.calculator.traits;

import examples.calculator.types;
import std;

namespace examples::calculator {

trait Tokeniser
{
    // Converts input string into a sequence of tokens
    tokenise(std::string_view input) const -> std::expected<std::vector<Token>, ParseError>
}

trait Parser
{
    // Converts a token sequence into an expression AST
    parse(std::span<Token const> tokens) const -> std::expected<ExprPtr, ParseError>
}

trait Evaluator
{
    // Evaluates an expression AST to produce a numeric result
    evaluate(Expression const& expr) -> std::expected<double, EvalError>
}

// Named variable storage
trait Variables
{
    get(std::string_view name) const -> std::optional<double>
    set(std::string name, double value)
    list() const -> std::vector<std::pair<std::string, double>>
}

// Registry of callable functions
trait Functions
{
    call(std::string_view name, std::span<double const> args) const -> std::expected<double, EvalError>
    list() const -> std::vector<std::string>
}

// Formats output for display
trait Formatter
{
    formatResult(double value) const -> std::string
    formatError(std::string_view message) const -> std::string
    formatAssignment(std::string_view name, double value) const -> std::string
    formatVariables(std::span<std::pair<std::string, double> const> vars) const -> std::string
    formatFunctions(std::span<std::string const> names) const -> std::string
}

}
