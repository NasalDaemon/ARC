module examples.calculator.parser;

import std;

namespace examples::calculator::node {

namespace {

// Helper to make ExprPtr
template<typename T>
auto makeExpr(T&& val) -> ExprPtr {
    return std::make_unique<Expression>(std::forward<T>(val));
}

struct ParserState {
    std::span<Token const> tokens;
    std::size_t pos = 0;

    Token const& peek() const {
        return tokens[pos];
    }

    Token const& advance() {
        return tokens[pos++];
    }

    // Parse expression (lowest precedence entry point, not top-level)
    auto parseExpr() -> std::expected<ExprPtr, ParseError>;
    auto parseAddSub() -> std::expected<ExprPtr, ParseError>;
    auto parseMulDiv() -> std::expected<ExprPtr, ParseError>;
    auto parsePow() -> std::expected<ExprPtr, ParseError>;
    auto parseUnary() -> std::expected<ExprPtr, ParseError>;
    auto parsePrimary() -> std::expected<ExprPtr, ParseError>;
};

auto ParserState::parseExpr() -> std::expected<ExprPtr, ParseError> {
    return parseAddSub();
}

auto ParserState::parseAddSub() -> std::expected<ExprPtr, ParseError> {
    auto left = parseMulDiv();
    if (!left) return left;

    while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
        BinaryOp op = (peek().type == TokenType::Plus) ? BinaryOp::Add : BinaryOp::Sub;
        advance();
        auto right = parseMulDiv();
        if (!right) return right;
        left = makeExpr(BinaryExpr{op, std::move(*left), std::move(*right)});
    }
    return left;
}

auto ParserState::parseMulDiv() -> std::expected<ExprPtr, ParseError> {
    auto left = parsePow();
    if (!left) return left;

    while (peek().type == TokenType::Star || peek().type == TokenType::Slash) {
        BinaryOp op = (peek().type == TokenType::Star) ? BinaryOp::Mul : BinaryOp::Div;
        advance();
        auto right = parsePow();
        if (!right) return right;
        left = makeExpr(BinaryExpr{op, std::move(*left), std::move(*right)});
    }
    return left;
}

auto ParserState::parsePow() -> std::expected<ExprPtr, ParseError> {
    auto base = parseUnary();
    if (!base) return base;

    // Right-associative: recurse into parsePow for the right operand
    if (peek().type == TokenType::Caret) {
        advance();
        auto exp = parsePow();
        if (!exp) return exp;
        return makeExpr(BinaryExpr{BinaryOp::Pow, std::move(*base), std::move(*exp)});
    }
    return base;
}

auto ParserState::parseUnary() -> std::expected<ExprPtr, ParseError> {
    if (peek().type == TokenType::Minus) {
        advance();
        auto operand = parseUnary();
        if (!operand) return operand;
        return makeExpr(UnaryExpr{UnaryOp::Negate, std::move(*operand)});
    }
    return parsePrimary();
}

auto ParserState::parsePrimary() -> std::expected<ExprPtr, ParseError> {
    Token const& tok = peek();

    if (tok.type == TokenType::Number) {
        advance();
        return makeExpr(NumberExpr{tok.numericValue});
    }

    if (tok.type == TokenType::Identifier) {
        std::string name{tok.text};
        advance();

        // Function call: identifier followed by '('
        if (peek().type == TokenType::LParen) {
            advance(); // consume '('
            std::vector<ExprPtr> args;

            if (peek().type != TokenType::RParen) {
                auto arg = parseExpr();
                if (!arg) return arg;
                args.push_back(std::move(*arg));

                while (peek().type == TokenType::Comma) {
                    advance(); // consume ','
                    auto nextArg = parseExpr();
                    if (!nextArg) return nextArg;
                    args.push_back(std::move(*nextArg));
                }
            }

            if (peek().type != TokenType::RParen) {
                return std::unexpected(ParseError{"expected ')' after function arguments", pos});
            }
            advance(); // consume ')'
            return makeExpr(CallExpr{std::move(name), std::move(args)});
        }

        // Variable reference
        return makeExpr(VariableExpr{std::move(name)});
    }

    if (tok.type == TokenType::LParen) {
        advance(); // consume '('
        auto expr = parseExpr();
        if (!expr) return expr;

        if (peek().type != TokenType::RParen) {
            return std::unexpected(ParseError{"expected ')'", pos});
        }
        advance(); // consume ')'
        return expr;
    }

    return std::unexpected(ParseError{
        std::string("unexpected token: ") + std::string(tok.text),
        pos
    });
}

} // anonymous namespace

auto Parser::parse(std::span<Token const> tokens, std::string_view originalLine) const
    -> std::expected<ExprPtr, ParseError>
{
    if (tokens.empty()) {
        return std::unexpected(ParseError{"empty token stream", 0});
    }

    // Check for function definition: identifier ( identifier, ... ) = expr
    if (tokens.size() >= 5 &&
        tokens[0].type == TokenType::Identifier &&
        tokens[1].type == TokenType::LParen)
    {
        std::size_t pos = 2;
        std::vector<std::string> params;
        bool isFuncDef = false;
        bool hasTrailingComma = false;

        if (tokens[pos].type == TokenType::Identifier)
        {
            params.emplace_back(tokens[pos].text);
            ++pos;
            while (pos < tokens.size() && tokens[pos].type == TokenType::Comma)
            {
                ++pos;
                hasTrailingComma = true;
                if (pos >= tokens.size() || tokens[pos].type != TokenType::Identifier)
                    break;
                params.emplace_back(tokens[pos].text);
                hasTrailingComma = false;
                ++pos;
            }
            if (!hasTrailingComma && pos < tokens.size() && tokens[pos].type == TokenType::RParen)
            {
                ++pos;
                if (pos < tokens.size() && tokens[pos].type == TokenType::Equals)
                {
                    ++pos;
                    isFuncDef = true;
                }
            }
        }

        if (isFuncDef)
        {
            std::string name{tokens[0].text};
            auto bodyTokens = tokens.subspan(pos);
            ParserState state{bodyTokens, 0};
            auto body = state.parseExpr();
            if (!body) return body;
            if (state.peek().type != TokenType::End)
                return std::unexpected(ParseError{"unexpected token after function body", state.pos});

            // Build source for the function body
            std::string source;
            if (!originalLine.empty() && !bodyTokens.empty())
            {
                char const* beginChar = bodyTokens.front().text.begin();
                char const* endChar = bodyTokens.back().text.end();
                if (beginChar >= originalLine.begin() && beginChar <= originalLine.end() &&
                    endChar >= originalLine.begin() && endChar <= originalLine.end())
                {
                    source.assign(beginChar, endChar);
                }
            }
            if (source.empty())
            {
                for (auto const& token : bodyTokens) {
                    if (token.type == TokenType::End) break;
                    if (!source.empty()) source += " ";
                    source += token.text;
                }
            }

            return makeExpr(FuncDefExpr{std::move(name), std::move(params), std::move(*body), std::move(source)});
        }
    }

    // Check for assignment: top-level identifier followed by '='
    if (tokens.size() >= 2 &&
        tokens[0].type == TokenType::Identifier &&
        tokens[1].type == TokenType::Equals)
    {
        std::string name{tokens[0].text};
        // Parse the RHS starting after the '='
        auto rhsTokens = tokens.subspan(2);
        ParserState state{rhsTokens, 0};
        auto rhs = state.parseExpr();
        if (!rhs) return rhs;
        if (state.peek().type != TokenType::End) {
            return std::unexpected(ParseError{"unexpected token after assignment", state.pos});
        }
        return makeExpr(AssignExpr{std::move(name), std::move(*rhs)});
    }

    ParserState state{tokens, 0};
    auto expr = state.parseExpr();
    if (!expr) return expr;

    // Should be at End token
    if (state.peek().type != TokenType::End) {
        return std::unexpected(ParseError{"unexpected token after expression", state.pos});
    }

    return expr;
}

} // namespace examples::calculator::node
