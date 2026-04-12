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

auto extractDouble(EvalResult const& r) -> double
{
    return std::visit([]<class T>(T const& v) -> double {
        if constexpr (std::is_same_v<T, FuncDefResult>)
            return 0.0;
        else
            return v.value;
    }, r);
}

EVALUATOR::evaluate(Expression const& expr) -> std::expected<EvalResult, EvalError>
{
    return std::visit([this]<class T>(T const& e) -> std::expected<EvalResult, EvalError> {
        if constexpr (std::is_same_v<T, NumberExpr>)
        {
            return NumberResult{e.value};
        }
        else if constexpr (std::is_same_v<T, VariableExpr>)
        {
            std::optional<double> val = getVariables().get(e.name);
            if (!val)
                return std::unexpected(EvalError{std::format("undefined variable: {}", e.name)});
            return NumberResult{*val};
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>)
        {
            auto operand = evaluate(*e.operand);
            if (!operand) return operand;
            switch (e.op)
            {
                case UnaryOp::Negate: return NumberResult{-extractDouble(*operand)};
            }
            return std::unexpected(EvalError{"unknown unary operator"});
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>)
        {
            auto left = evaluate(*e.left);
            if (!left) return left;
            auto right = evaluate(*e.right);
            if (!right) return right;
            double lv = extractDouble(*left);
            double rv = extractDouble(*right);
            switch (e.op)
            {
                case BinaryOp::Add: return NumberResult{lv + rv};
                case BinaryOp::Sub: return NumberResult{lv - rv};
                case BinaryOp::Mul: return NumberResult{lv * rv};
                case BinaryOp::Div:
                    if (rv == 0.0)
                        return std::unexpected(EvalError{"division by zero"});
                    return NumberResult{lv / rv};
                case BinaryOp::Pow: return NumberResult{std::pow(lv, rv)};
            }
            return std::unexpected(EvalError{"unknown binary operator"});
        }
        else if constexpr (std::is_same_v<T, AssignExpr>)
        {
            auto val = evaluate(*e.value);
            if (!val) return val;
            double dv = extractDouble(*val);
            getVariables().set(e.name, dv);
            return AssignResult{e.name, dv};
        }
        else if constexpr (std::is_same_v<T, CallExpr>)
        {
            std::vector<double> args;
            args.reserve(e.args.size());
            for (auto const& argExpr : e.args)
            {
                auto argVal = evaluate(*argExpr);
                if (!argVal) return argVal;
                args.push_back(extractDouble(*argVal));
            }

            auto result = getFunctions().call(e.name, args);
            if (result.has_value())
                return NumberResult{*result};

            auto const* userFunc = getFunctions().getUserFunction(e.name, args.size());
            if (!userFunc)
                return std::unexpected(result.error());

            std::vector<std::optional<double>> savedVars;
            savedVars.reserve(userFunc->params.size());
            for (std::size_t i = 0; i < userFunc->params.size(); ++i)
            {
                savedVars.push_back(getVariables().get(userFunc->params[i]));
                getVariables().set(userFunc->params[i], args[i]);
            }

            auto guard = arc::Defer(
                [&] {
                    for (std::size_t i = 0; i < userFunc->params.size(); ++i)
                    {
                        if (savedVars[i].has_value())
                            getVariables().set(userFunc->params[i], *savedVars[i]);
                        else
                            getVariables().remove(userFunc->params[i]);
                    }
                });

            auto bodyResult = evaluate(*userFunc->body);
            if (!bodyResult) return bodyResult;
            return NumberResult{extractDouble(*bodyResult)};
        }
        else if constexpr (std::is_same_v<T, FuncDefExpr>)
        {
            auto result = getFunctions().define(e.name, e.params, e.body->clone(), e.source);
            if (!result.has_value())
                return std::unexpected(result.error());
            return FuncDefResult{e.name, e.params};
        }
    }, static_cast<Expression::variant const&>(expr));
}

} // namespace examples::calculator::node
