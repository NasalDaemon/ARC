export module examples.calculator.parser;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Parser : arc::NodeImpl<trait::Parser>
{
    auto parse(std::span<Token const> tokens, std::string_view originalLine) const -> std::expected<ExprPtr, ParseError>;
};

} // namespace examples::calculator::node
