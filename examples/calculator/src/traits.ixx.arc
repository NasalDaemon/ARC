export module examples.calculator.traits;

import examples.calculator.types;
import std;

namespace examples::calculator {

trait Tokeniser
{
    tokenise(std::string_view input) const -> std::expected<std::vector<Token>, ParseError>
        pre(not input.empty())
}

trait Parser
{
    parse(std::span<Token const> tokens, std::string_view originalLine) const -> std::expected<ExprPtr, ParseError>
        pre(not tokens.empty())
}

trait Evaluator
{
    evaluate(Expression const& expr) -> std::expected<EvalResult, EvalError>
}

trait Variables
{
    get(std::string_view name) const -> std::optional<double>
        pre(not name.empty())
    set(std::string name, double value)
        pre(not name.empty())
    remove(std::string_view name) -> bool
        pre(not name.empty())
    list() const -> std::vector<std::pair<std::string, double>>
    clear()
        post(self.list().empty())
}

// Builtin function dispatch and listing.
trait BuiltinFunctions
{
    // call() returns the result for known builtins, or an error if the name/arity is unknown or mismatched
    call(std::string_view name, std::span<double const> args) const -> std::expected<double, EvalError>
        pre(not name.empty())
    list() const -> std::vector<std::string>
}

// User-defined function registry: definition, removal, and inspection.
trait UserFunctions
{
    define(std::span<FunctionDefinition const> functions) -> std::expected<void, EvalError>
        pre(not functions.empty())
    define(std::string name, std::vector<std::string> params, ExprPtr body, std::string source) -> std::expected<void, EvalError>
        pre(not name.empty())
        pre(body != nullptr)

    list() const -> std::vector<std::pair<std::string, UserFunction const*>>
    get(std::string_view name, std::size_t arity) const -> UserFunction const*
        pre(not name.empty())

    remove(std::span<std::string const> names) -> std::expected<void, EvalError>
        pre(not names.empty())
    clear() -> void
        post(self.list().empty())
}

trait Functions = BuiltinFunctions + UserFunctions

trait Formatter
{
    formatResult(double value) const -> std::string
        post(result: not result.empty())
    formatError(std::string_view message) const -> std::string
        pre(not message.empty())
    formatAssignment(std::string_view name, double value) const -> std::string
        pre(not name.empty())
    formatVariables(std::span<std::pair<std::string, double> const> vars) const -> std::string
    formatFunctions(std::span<std::string const> builtins, std::span<std::pair<std::string, UserFunction const*> const> userFuncs) const -> std::string
    formatFunctionDef(std::string_view name, std::span<std::string const> params) const -> std::string
        pre(not name.empty())
}

// Abstracts line input for testability and mode switching.
// Returns nullopt on EOF (end of input).
trait LineReader
{
    readLine(std::string_view prompt) -> std::optional<std::string>
}

// Abstracts output for testability.
// writeLine appends a newline; write does not.
trait Output
{
    write(std::string_view text) -> void
    writeLine(std::string_view text) -> void
}

// Expression history storage.
// Used by Repl to record entries, by TerminalLineReader for up/down navigation,
// and by CommandHandler for the "history" command.
trait History
{
    addEntry(std::string line) -> void
    entries() const -> std::vector<std::string>
}

// Command dispatch.
// Protocol: call isCommand to check if the first word matches a known command,
// then call execute only if isCommand returned true.
trait Commands
{
    isCommand(std::string_view input) const -> bool
    execute(std::string_view input) -> std::expected<std::string, std::string>
        pre(self.isCommand(input))
}

// Save/load calculator state (variables and user-defined functions) to/from a file.
trait Persistence
{
    save(std::string_view path) -> std::expected<void, std::string>
    load(std::string_view path) -> std::expected<void, std::string>
}

} // namespace examples::calculator
