export module examples.calculator.types;
import std;

namespace examples::calculator {

// --- Tokens ---

export enum class TokenType
{
    Number,      // 3.14
    Identifier,  // x, sqrt, ans
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Caret,       // ^
    LParen,      // (
    RParen,      // )
    Comma,       // ,
    Equals,      // =
    End,         // end of input
};

export struct Token
{
    TokenType type;
    std::string_view text;  // view into the original input
    double numericValue{};  // populated for Number tokens
};

// --- AST ---

export enum class BinaryOp { Add, Sub, Mul, Div, Pow };
export enum class UnaryOp  { Negate };

export struct Expression;

export using ExprPtr = std::unique_ptr<Expression>;

export struct NumberExpr    { double value; };
export struct VariableExpr  { std::string name; };
export struct UnaryExpr     { UnaryOp op; ExprPtr operand; };
export struct BinaryExpr    { BinaryOp op; ExprPtr left; ExprPtr right; };
export struct AssignExpr    { std::string name; ExprPtr value; };
export struct CallExpr      { std::string name; std::vector<ExprPtr> args; };
export struct FuncDefExpr   { std::string name; std::vector<std::string> params; ExprPtr body; std::string source; };

export struct Expression : std::variant<
    NumberExpr, VariableExpr, UnaryExpr, BinaryExpr, AssignExpr, CallExpr, FuncDefExpr>
{
    using variant::variant;

    auto callDependencies() const -> std::set<std::pair<std::string, std::size_t>>;
    auto variableReferences() const -> std::set<std::string>;
    auto clone() const -> ExprPtr;
};

// --- User Functions ---

export struct UserFunction
{
    std::vector<std::string> params;
    ExprPtr body;
    std::string source; // original body text for persistence, e.g. "x ^ 2 + 1"
};

// Used as input to Functions::define for single or batch function definition.
// Copyable (copy-constructs a deep clone of body) so it can be passed through mock recording.
export struct FunctionDefinition
{
    std::string name;
    std::vector<std::string> params;
    ExprPtr body;
    std::string source;

    FunctionDefinition() = default;
    FunctionDefinition(std::string name, std::vector<std::string> params, ExprPtr body, std::string source)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)), source(std::move(source)) {}
    FunctionDefinition(FunctionDefinition&&) = default;
    FunctionDefinition& operator=(FunctionDefinition&&) = default;
    FunctionDefinition(FunctionDefinition const& o)
        : name(o.name), params(o.params), body(o.body ? o.body->clone() : nullptr), source(o.source) {}
    FunctionDefinition& operator=(FunctionDefinition const& o)
    {
        name = o.name; params = o.params;
        body = o.body ? o.body->clone() : nullptr;
        source = o.source;
        return *this;
    }
};

// --- Errors ---

export struct ParseError
{
    std::string message;
    std::size_t position{};  // offset into source
};

export struct EvalError
{
    std::string message;
};

} // namespace examples::calculator
