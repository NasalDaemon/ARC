module examples.calculator.evaluator:impl;

import examples.calculator.evaluator;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#define EVALUATOR \
    template<class Context> \
    auto Evaluator::Node<Context>

namespace examples::calculator::node {

EVALUATOR::evaluate(Expression const& expr) -> std::expected<double, EvalError>
{
    return std::visit([this](auto const& e) -> std::expected<double, EvalError> {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, NumberExpr>)
        {
            return e.value;
        }
        else if constexpr (std::is_same_v<T, VariableExpr>)
        {
            std::optional<double> val = getVariables().get(e.name);
            if (!val)
                return std::unexpected(EvalError{"undefined variable: " + e.name});
            return *val;
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>)
        {
            auto operand = evaluate(*e.operand);
            if (!operand) return operand;
            switch (e.op)
            {
                case UnaryOp::Negate: return -*operand;
            }
            return std::unexpected(EvalError{"unknown unary operator"});
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>)
        {
            auto left = evaluate(*e.left);
            if (!left) return left;
            auto right = evaluate(*e.right);
            if (!right) return right;

            switch (e.op)
            {
                case BinaryOp::Add: return *left + *right;
                case BinaryOp::Sub: return *left - *right;
                case BinaryOp::Mul: return *left * *right;
                case BinaryOp::Div:
                    if (*right == 0.0)
                        return std::unexpected(EvalError{"division by zero"});
                    return *left / *right;
                case BinaryOp::Pow: return std::pow(*left, *right);
            }
            return std::unexpected(EvalError{"unknown binary operator"});
        }
        else if constexpr (std::is_same_v<T, AssignExpr>)
        {
            auto val = evaluate(*e.value);
            if (!val) return val;
            getVariables().set(e.name, *val);
            return *val;
        }
        else if constexpr (std::is_same_v<T, CallExpr>)
        {
            std::vector<double> args;
            args.reserve(e.args.size());
            for (auto const& argExpr : e.args)
            {
                auto argVal = evaluate(*argExpr);
                if (!argVal) return argVal;
                args.push_back(*argVal);
            }
            return getFunctions().call(e.name, args);
        }
    }, static_cast<Expression::variant const&>(expr));
}

}
