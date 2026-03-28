export module examples.calculator.tokeniser;

import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Tokeniser : arc::NodeImpl<trait::Tokeniser>
{
    auto tokenise(std::string_view input) const -> std::expected<std::vector<Token>, ParseError>;
};

} // namespace examples::calculator::node
