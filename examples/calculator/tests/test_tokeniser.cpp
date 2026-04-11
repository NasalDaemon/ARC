#include <doctest/doctest.h>

import examples.calculator.tokeniser;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;
using namespace std::string_view_literals;

SCENARIO("Tokenising numeric literals")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"42\"")
        {
            auto result = tokeniser.tokenise("42"sv);

            THEN("produces a single Number token with value 42")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 2); // Number + End
                CHECK((*result)[0].type == TokenType::Number);
                CHECK((*result)[0].text == "42");
                CHECK((*result)[0].numericValue == doctest::Approx(42.0));
                CHECK((*result)[1].type == TokenType::End);
            }
        }
        WHEN("tokenising \"3.14\"")
        {
            auto result = tokeniser.tokenise("3.14"sv);

            THEN("produces a single Number token with value 3.14")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 2);
                CHECK((*result)[0].type == TokenType::Number);
                CHECK((*result)[0].text == "3.14");
                CHECK((*result)[0].numericValue == doctest::Approx(3.14));
                CHECK((*result)[1].type == TokenType::End);
            }
        }
        WHEN("tokenising \".5\"")
        {
            auto result = tokeniser.tokenise(".5"sv);

            THEN("produces a single Number token with value 0.5")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 2);
                CHECK((*result)[0].type == TokenType::Number);
                CHECK((*result)[0].text == ".5");
                CHECK((*result)[0].numericValue == doctest::Approx(0.5));
                CHECK((*result)[1].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising operators")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"+-*/^\"")
        {
            auto result = tokeniser.tokenise("+-*/^"sv);

            THEN("produces Plus, Minus, Star, Slash, Caret tokens in order")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 6); // 5 operators + End
                CHECK((*result)[0].type == TokenType::Plus);
                CHECK((*result)[1].type == TokenType::Minus);
                CHECK((*result)[2].type == TokenType::Star);
                CHECK((*result)[3].type == TokenType::Slash);
                CHECK((*result)[4].type == TokenType::Caret);
                CHECK((*result)[5].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising parentheses and comma")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"(a, b)\"")
        {
            auto result = tokeniser.tokenise("(a, b)"sv);

            THEN("produces LParen, Identifier, Comma, Identifier, RParen tokens")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 6); // LParen, a, Comma, b, RParen, End
                CHECK((*result)[0].type == TokenType::LParen);
                CHECK((*result)[1].type == TokenType::Identifier);
                CHECK((*result)[1].text == "a");
                CHECK((*result)[2].type == TokenType::Comma);
                CHECK((*result)[3].type == TokenType::Identifier);
                CHECK((*result)[3].text == "b");
                CHECK((*result)[4].type == TokenType::RParen);
                CHECK((*result)[5].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising identifiers")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"x\"")
        {
            auto result = tokeniser.tokenise("x"sv);

            THEN("produces a single Identifier token with text \"x\"")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 2);
                CHECK((*result)[0].type == TokenType::Identifier);
                CHECK((*result)[0].text == "x");
                CHECK((*result)[1].type == TokenType::End);
            }
        }
        WHEN("tokenising \"sqrt\"")
        {
            auto result = tokeniser.tokenise("sqrt"sv);

            THEN("produces a single Identifier token with text \"sqrt\"")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 2);
                CHECK((*result)[0].type == TokenType::Identifier);
                CHECK((*result)[0].text == "sqrt");
                CHECK((*result)[1].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising assignment")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"x = 5\"")
        {
            auto result = tokeniser.tokenise("x = 5"sv);

            THEN("produces Identifier, Equals, Number tokens")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 4); // Identifier, Equals, Number, End
                CHECK((*result)[0].type == TokenType::Identifier);
                CHECK((*result)[0].text == "x");
                CHECK((*result)[1].type == TokenType::Equals);
                CHECK((*result)[2].type == TokenType::Number);
                CHECK((*result)[2].numericValue == doctest::Approx(5.0));
                CHECK((*result)[3].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising complex expressions")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"2 + 3 * (4 - 1)\"")
        {
            auto result = tokeniser.tokenise("2 + 3 * (4 - 1)"sv);

            THEN("produces correct token sequence with End")
            {
                REQUIRE(result.has_value());
                // 2, +, 3, *, (, 4, -, 1, ), End = 10 tokens
                REQUIRE(result->size() == 10);
                CHECK((*result)[0].type == TokenType::Number);
                CHECK((*result)[0].numericValue == doctest::Approx(2.0));
                CHECK((*result)[1].type == TokenType::Plus);
                CHECK((*result)[2].type == TokenType::Number);
                CHECK((*result)[2].numericValue == doctest::Approx(3.0));
                CHECK((*result)[3].type == TokenType::Star);
                CHECK((*result)[4].type == TokenType::LParen);
                CHECK((*result)[5].type == TokenType::Number);
                CHECK((*result)[5].numericValue == doctest::Approx(4.0));
                CHECK((*result)[6].type == TokenType::Minus);
                CHECK((*result)[7].type == TokenType::Number);
                CHECK((*result)[7].numericValue == doctest::Approx(1.0));
                CHECK((*result)[8].type == TokenType::RParen);
                CHECK((*result)[9].type == TokenType::End);
            }
        }
    }
}

SCENARIO("Tokenising errors")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("tokenising \"&\"")
        {
            auto result = tokeniser.tokenise("&"sv);

            THEN("returns a ParseError with position")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().position == 0);
            }
        }
    }
}

SCENARIO("Tokeniser contract: tokenise rejects empty input")
{
    arc::test::Graph<node::Tokeniser> graph;
    auto tokeniser = graph.node.asTrait(trait::tokeniser);

    GIVEN("a Tokeniser node")
    {
        WHEN("calling tokenise with an empty string")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(tokeniser.tokenise(""sv), arc::ContractViolation);
            }
        }
    }
}
