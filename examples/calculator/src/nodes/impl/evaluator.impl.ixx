module examples.calculator.node.evaluator:impl;

import examples.calculator.node.evaluator;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#define EVALUATOR \
    template<class Context> \
    auto Evaluator::Node<Context>

namespace examples::calculator::node {

EVALUATOR::evalExpr(Expression const& expr) -> std::expected<double, EvalError>
{
    return std::visit([this]<class T>(T const& e) -> std::expected<double, EvalError> {
        if constexpr (std::is_same_v<T, NumberExpr>)
        {
            return e.value;
        }
        else if constexpr (std::is_same_v<T, VariableExpr>)
        {
            std::optional<double> val = getVariables().get(e.name);
            if (!val)
                return std::unexpected(EvalError{std::format("undefined variable: {}", e.name)});
            return *val;
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>)
        {
            auto operand = evalExpr(*e.operand);
            if (!operand) return operand;
            switch (e.op)
            {
                case UnaryOp::Negate: return -*operand;
            }
            return std::unexpected(EvalError{"unknown unary operator"});
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>)
        {
            auto left = evalExpr(*e.left);
            if (!left) return left;
            auto right = evalExpr(*e.right);
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
        else if constexpr (std::is_same_v<T, CallExpr>)
        {
            std::vector<double> args;
            args.reserve(e.args.size());
            for (auto const& argExpr : e.args)
            {
                auto argVal = evalExpr(*argExpr);
                if (!argVal) return argVal;
                args.push_back(*argVal);
            }

            // Try builtin dispatch first.
            auto builtinResult = getBuiltinFunctions().call(e.name, args);
            if (builtinResult)
                return *builtinResult;

            // Fall back to user-defined function.
            auto const* userFunc = getUserFunctions().get(e.name, args.size());
            if (!userFunc)
                return std::unexpected(EvalError{std::format("unknown function: {}/{}", e.name, args.size())});

            // Save current variable values, bind parameters, evaluate body, then restore.
            std::vector<std::optional<double>> savedVars;
            savedVars.reserve(userFunc->params.size());
            for (std::size_t i = 0; i < userFunc->params.size(); ++i)
            {
                savedVars.push_back(getVariables().get(userFunc->params[i]));
                getVariables().set(userFunc->params[i], args[i]);
            }

            auto guard = arc::Defer([&] {
                for (std::size_t i = 0; i < userFunc->params.size(); ++i)
                {
                    if (savedVars[i].has_value())
                        getVariables().set(userFunc->params[i], *savedVars[i]);
                    else
                        getVariables().remove(userFunc->params[i]);
                }
            });

            return evalExpr(*userFunc->body);
        }
        else
        {
            return std::unexpected(EvalError{"unexpected expression type"});
        }
    }, static_cast<Expression::variant const&>(expr));
}

EVALUATOR::evaluate(Expression const& expr) -> std::expected<EvalResult, EvalError>
{
    return std::visit([this, &expr]<class T>(T const& e) -> std::expected<EvalResult, EvalError> {
        if constexpr (std::is_same_v<T, AssignExpr>)
        {
            auto val = evalExpr(*e.value);
            if (!val) return std::unexpected(val.error());
            getVariables().set(e.name, *val);
            getVariables().set(std::string{"ans"}, *val);
            return AssignResult{e.name, *val};
        }
        else if constexpr (std::is_same_v<T, FuncDefExpr>)
        {
            auto result = getUserFunctions().define(e.name, e.params, e.body->clone(), e.source);
            if (!result.has_value())
                return std::unexpected(result.error());
            return FuncDefResult{e.name, e.params};
        }
        else
        {
            auto val = evalExpr(expr);
            if (!val) return std::unexpected(val.error());
            getVariables().set(std::string{"ans"}, *val);
            return NumberResult{*val};
        }
    }, static_cast<Expression::variant const&>(expr));
}

} // namespace examples::calculator::node
