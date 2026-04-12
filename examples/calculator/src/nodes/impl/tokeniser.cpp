module examples.calculator.node.tokeniser;

import std;

namespace examples::calculator::node {

auto Tokeniser::tokenise(std::string_view input) const
    -> std::expected<std::vector<Token>, ParseError>
{
    std::vector<Token> tokens;
    std::size_t pos = 0;

    while (pos < input.size()) {
        // Skip whitespace
        if (std::isspace(static_cast<unsigned char>(input[pos]))) {
            ++pos;
            continue;
        }

        char c = input[pos];

        // Number: digits, optional decimal point, or leading dot
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::size_t start = pos;
            // Consume digits before decimal point
            while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
                ++pos;
            }
            // Consume decimal point and digits after it
            if (pos < input.size() && input[pos] == '.') {
                ++pos;
                while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
                    ++pos;
                }
            }
            std::string_view text = input.substr(start, pos - start);
            double value = 0.0;
            std::from_chars(text.data(), text.data() + text.size(), value);
            tokens.push_back({TokenType::Number, text, value});
            continue;
        }

        // Identifier: [a-zA-Z_][a-zA-Z0-9_]*
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t start = pos;
            while (pos < input.size() &&
                   (std::isalnum(static_cast<unsigned char>(input[pos])) || input[pos] == '_')) {
                ++pos;
            }
            tokens.push_back({TokenType::Identifier, input.substr(start, pos - start), 0.0});
            continue;
        }

        // Single-character tokens
        TokenType type;
        switch (c) {
            case '+': type = TokenType::Plus;   break;
            case '-': type = TokenType::Minus;  break;
            case '*': type = TokenType::Star;   break;
            case '/': type = TokenType::Slash;  break;
            case '^': type = TokenType::Caret;  break;
            case '(': type = TokenType::LParen; break;
            case ')': type = TokenType::RParen; break;
            case ',': type = TokenType::Comma;  break;
            case '=': type = TokenType::Equals; break;
            default:
                return std::unexpected(ParseError{
                    std::string("unexpected character: ") + c,
                    pos
                });
        }
        tokens.push_back({type, input.substr(pos, 1), 0.0});
        ++pos;
    }

    tokens.push_back({TokenType::End, input.substr(input.size(), 0), 0.0});
    return tokens;
}

} // namespace examples::calculator::node
