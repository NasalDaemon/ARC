module examples.calculator.types;

import std;

namespace examples::calculator {

namespace {

template<typename T>
auto makeExpr(T&& val) -> ExprPtr
{
    return std::make_unique<Expression>(std::forward<T>(val));
}

} // namespace

auto Expression::clone() const -> ExprPtr
{
    return std::visit([](auto const& e) -> ExprPtr {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, NumberExpr>)
            return makeExpr(NumberExpr{e.value});
        else if constexpr (std::is_same_v<T, VariableExpr>)
            return makeExpr(VariableExpr{e.name});
        else if constexpr (std::is_same_v<T, UnaryExpr>)
            return makeExpr(UnaryExpr{e.op, e.operand->clone()});
        else if constexpr (std::is_same_v<T, BinaryExpr>)
            return makeExpr(BinaryExpr{e.op, e.left->clone(), e.right->clone()});
        else if constexpr (std::is_same_v<T, AssignExpr>)
            return makeExpr(AssignExpr{e.name, e.value->clone()});
        else if constexpr (std::is_same_v<T, CallExpr>)
        {
            std::vector<ExprPtr> args;
            args.reserve(e.args.size());
            for (auto const& arg : e.args)
                args.push_back(arg->clone());
            return makeExpr(CallExpr{e.name, std::move(args)});
        }
        else if constexpr (std::is_same_v<T, FuncDefExpr>)
        {
            std::vector<std::string> params{e.params};
            return makeExpr(FuncDefExpr{e.name, std::move(params), e.body->clone(), e.source});
        }
    }, static_cast<Expression::variant const&>(*this));
}

auto Expression::callDependencies() const -> std::set<std::pair<std::string, std::size_t>>
{
    std::set<std::pair<std::string, std::size_t>> deps;

    auto const visit = [&](this auto const& self, Expression const& e) -> void {
        std::visit([&](auto const& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, CallExpr>)
            {
                deps.insert({v.name, v.args.size()});
                for (auto const& arg : v.args) self(*arg);
            }
            else if constexpr (std::is_same_v<T, UnaryExpr>)  { self(*v.operand); }
            else if constexpr (std::is_same_v<T, BinaryExpr>) { self(*v.left); self(*v.right); }
            else if constexpr (std::is_same_v<T, AssignExpr>) { self(*v.value); }
            else if constexpr (std::is_same_v<T, FuncDefExpr>){ self(*v.body); }
        }, static_cast<Expression::variant const&>(e));
    };

    visit(*this);
    return deps;
}

auto Expression::variableReferences() const -> std::set<std::string>
{
    std::set<std::string> refs;

    auto const visit = [&](this auto const& self, Expression const& e) -> void {
        std::visit([&](auto const& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, VariableExpr>)
                refs.insert(v.name);
            else if constexpr (std::is_same_v<T, CallExpr>)
            {
                for (auto const& arg : v.args) self(*arg);
            }
            else if constexpr (std::is_same_v<T, UnaryExpr>)  { self(*v.operand); }
            else if constexpr (std::is_same_v<T, BinaryExpr>) { self(*v.left); self(*v.right); }
            else if constexpr (std::is_same_v<T, AssignExpr>) { self(*v.value); }
            else if constexpr (std::is_same_v<T, FuncDefExpr>){ self(*v.body); }
        }, static_cast<Expression::variant const&>(e));
    };

    visit(*this);
    return refs;
}

} // namespace examples::calculator
