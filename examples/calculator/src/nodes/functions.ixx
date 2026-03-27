export module examples.calculator.functions;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Functions : arc::NodeImpl<trait::Functions>
{
    auto call(std::string_view name, std::span<double const> args) const -> std::expected<double, EvalError>;
    auto list() const -> std::vector<std::string>;
};

}
