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

export struct Expression : std::variant<
    NumberExpr, VariableExpr, UnaryExpr, BinaryExpr, AssignExpr, CallExpr>
{
    using variant::variant;
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

}
