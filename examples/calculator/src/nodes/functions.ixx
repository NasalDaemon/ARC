export module examples.calculator.node.functions;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Functions
{
    using FuncKey = std::pair<std::string, std::size_t>;

    template<class Context>
    struct Node : arc::Node::
        Impl<trait::Functions>::
        Uses<Variables>
    {
        using Methods::impl;
        using BuiltinFunctions::Disable::list;
        using UserFunctions::Disable::list;

        // BuiltinFunctions
        auto call(std::string_view name, std::span<double const> args) const -> std::expected<double, EvalError>;
        auto impl(BuiltinFunctions::list) const -> std::vector<std::string>;

        // UserFunctions
        auto define(std::span<FunctionDefinition const> functions) -> std::expected<void, EvalError>;
        auto define(std::string name, std::vector<std::string> params, ExprPtr body, std::string source) -> std::expected<void, EvalError>;
        auto impl(UserFunctions::list) const -> std::vector<std::pair<std::string, UserFunction const*>>;
        auto get(std::string_view name, std::size_t arity) const -> UserFunction const*;
        auto remove(std::span<std::string const> names) -> std::expected<void, EvalError>;
        void clear();

    private:
        std::map<FuncKey, UserFunction, std::less<>> userFunctions_;
        std::map<FuncKey, std::set<FuncKey>, std::less<>> dependencies_;
    };
};

} // namespace examples::calculator::node
