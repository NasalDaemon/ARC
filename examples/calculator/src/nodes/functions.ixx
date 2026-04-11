export module examples.calculator.functions;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Functions
{
    template<class Context>
    struct Node : arc::Node::
        Impl<trait::Functions>::
        Uses<trait::Variables>
    {
        auto call(std::string_view name, std::span<double const> args) const -> std::expected<double, EvalError>;
        auto listBuiltins() const -> std::vector<std::string>;
        auto define(std::span<FunctionDefinition const> functions) -> std::expected<void, EvalError>;
        auto define(std::string name, std::vector<std::string> params, ExprPtr body, std::string source) -> std::expected<void, EvalError>;
        auto removeFunctions(std::span<std::string const> names) -> std::expected<void, EvalError>;
        auto getUserFunction(std::string_view name, std::size_t arity) const -> UserFunction const*;
        auto listUserFunctions() const -> std::vector<std::pair<std::string, UserFunction const*>>;
        void clearUserFunctions();

    private:
        std::map<std::pair<std::string, std::size_t>, UserFunction, std::less<>> userFunctions_;
        std::map<std::pair<std::string, std::size_t>, std::set<std::pair<std::string, std::size_t>>, std::less<>> dependencies_;
    };
};

} // namespace examples::calculator::node
