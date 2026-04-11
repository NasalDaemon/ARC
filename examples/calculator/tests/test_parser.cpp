#include <doctest/doctest.h>

import examples.calculator.parser;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("Parsing numeric literals")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing tokens for \"42\"")
        {
            std::vector<Token> tokens{
                {TokenType::Number, "42", 42.0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a NumberExpr with value 42")
            {
                REQUIRE(result.has_value());
                auto* num = std::get_if<NumberExpr>(result->get());
                REQUIRE(num != nullptr);
                CHECK(num->value == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Parsing variable references")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing tokens for \"x\"")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "x", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a VariableExpr with name \"x\"")
            {
                REQUIRE(result.has_value());
                auto* var = std::get_if<VariableExpr>(result->get());
                REQUIRE(var != nullptr);
                CHECK(var->name == "x");
            }
        }
    }
}

SCENARIO("Parsing binary operations with BIDMAS precedence")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"2 + 3 * 4\"")
        {
            std::vector<Token> tokens{
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "3", 3},
                {TokenType::Star, "*", 0},
                {TokenType::Number, "4", 4},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Add(2, Mul(3, 4))")
            {
                REQUIRE(result.has_value());
                // Root should be BinaryExpr Add
                auto* add = std::get_if<BinaryExpr>(result->get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                // Left should be NumberExpr(2)
                auto* left = std::get_if<NumberExpr>(add->left.get());
                REQUIRE(left != nullptr);
                CHECK(left->value == doctest::Approx(2.0));
                // Right should be BinaryExpr Mul
                auto* mul = std::get_if<BinaryExpr>(add->right.get());
                REQUIRE(mul != nullptr);
                CHECK(mul->op == BinaryOp::Mul);
                auto* mulLeft = std::get_if<NumberExpr>(mul->left.get());
                REQUIRE(mulLeft != nullptr);
                CHECK(mulLeft->value == doctest::Approx(3.0));
                auto* mulRight = std::get_if<NumberExpr>(mul->right.get());
                REQUIRE(mulRight != nullptr);
                CHECK(mulRight->value == doctest::Approx(4.0));
            }
        }
        WHEN("parsing \"2 ^ 3 ^ 2\"")
        {
            std::vector<Token> tokens{
                {TokenType::Number, "2", 2},
                {TokenType::Caret, "^", 0},
                {TokenType::Number, "3", 3},
                {TokenType::Caret, "^", 0},
                {TokenType::Number, "2", 2},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Pow(2, Pow(3, 2)) — right-associative")
            {
                REQUIRE(result.has_value());
                // Root: Pow(2, ...)
                auto* pow1 = std::get_if<BinaryExpr>(result->get());
                REQUIRE(pow1 != nullptr);
                CHECK(pow1->op == BinaryOp::Pow);
                auto* base = std::get_if<NumberExpr>(pow1->left.get());
                REQUIRE(base != nullptr);
                CHECK(base->value == doctest::Approx(2.0));
                // Right: Pow(3, 2)
                auto* pow2 = std::get_if<BinaryExpr>(pow1->right.get());
                REQUIRE(pow2 != nullptr);
                CHECK(pow2->op == BinaryOp::Pow);
                auto* pow2Left = std::get_if<NumberExpr>(pow2->left.get());
                REQUIRE(pow2Left != nullptr);
                CHECK(pow2Left->value == doctest::Approx(3.0));
                auto* pow2Right = std::get_if<NumberExpr>(pow2->right.get());
                REQUIRE(pow2Right != nullptr);
                CHECK(pow2Right->value == doctest::Approx(2.0));
            }
        }
        WHEN("parsing \"10 - 3 - 2\"")
        {
            std::vector<Token> tokens{
                {TokenType::Number, "10", 10},
                {TokenType::Minus, "-", 0},
                {TokenType::Number, "3", 3},
                {TokenType::Minus, "-", 0},
                {TokenType::Number, "2", 2},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Sub(Sub(10, 3), 2) — left-associative")
            {
                REQUIRE(result.has_value());
                // Root: Sub(Sub(10,3), 2)
                auto* sub1 = std::get_if<BinaryExpr>(result->get());
                REQUIRE(sub1 != nullptr);
                CHECK(sub1->op == BinaryOp::Sub);
                // Left: Sub(10, 3)
                auto* sub2 = std::get_if<BinaryExpr>(sub1->left.get());
                REQUIRE(sub2 != nullptr);
                CHECK(sub2->op == BinaryOp::Sub);
                auto* sub2Left = std::get_if<NumberExpr>(sub2->left.get());
                REQUIRE(sub2Left != nullptr);
                CHECK(sub2Left->value == doctest::Approx(10.0));
                auto* sub2Right = std::get_if<NumberExpr>(sub2->right.get());
                REQUIRE(sub2Right != nullptr);
                CHECK(sub2Right->value == doctest::Approx(3.0));
                // Right: 2
                auto* right = std::get_if<NumberExpr>(sub1->right.get());
                REQUIRE(right != nullptr);
                CHECK(right->value == doctest::Approx(2.0));
            }
        }
    }
}

SCENARIO("Parsing parenthesised expressions")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"(2 + 3) * 4\"")
        {
            std::vector<Token> tokens{
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "3", 3},
                {TokenType::RParen, ")", 0},
                {TokenType::Star, "*", 0},
                {TokenType::Number, "4", 4},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Mul(Add(2, 3), 4)")
            {
                REQUIRE(result.has_value());
                // Root: Mul
                auto* mul = std::get_if<BinaryExpr>(result->get());
                REQUIRE(mul != nullptr);
                CHECK(mul->op == BinaryOp::Mul);
                // Left: Add(2, 3)
                auto* add = std::get_if<BinaryExpr>(mul->left.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                auto* addLeft = std::get_if<NumberExpr>(add->left.get());
                REQUIRE(addLeft != nullptr);
                CHECK(addLeft->value == doctest::Approx(2.0));
                auto* addRight = std::get_if<NumberExpr>(add->right.get());
                REQUIRE(addRight != nullptr);
                CHECK(addRight->value == doctest::Approx(3.0));
                // Right: 4
                auto* right = std::get_if<NumberExpr>(mul->right.get());
                REQUIRE(right != nullptr);
                CHECK(right->value == doctest::Approx(4.0));
            }
        }
    }
}

SCENARIO("Parsing unary negation")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"-5\"")
        {
            std::vector<Token> tokens{
                {TokenType::Minus, "-", 0},
                {TokenType::Number, "5", 5},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Negate(5)")
            {
                REQUIRE(result.has_value());
                auto* unary = std::get_if<UnaryExpr>(result->get());
                REQUIRE(unary != nullptr);
                CHECK(unary->op == UnaryOp::Negate);
                auto* operand = std::get_if<NumberExpr>(unary->operand.get());
                REQUIRE(operand != nullptr);
                CHECK(operand->value == doctest::Approx(5.0));
            }
        }
        WHEN("parsing \"-(2 + 3)\"")
        {
            std::vector<Token> tokens{
                {TokenType::Minus, "-", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "3", 3},
                {TokenType::RParen, ")", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces Negate(Add(2, 3))")
            {
                REQUIRE(result.has_value());
                auto* unary = std::get_if<UnaryExpr>(result->get());
                REQUIRE(unary != nullptr);
                CHECK(unary->op == UnaryOp::Negate);
                auto* add = std::get_if<BinaryExpr>(unary->operand.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                auto* addLeft = std::get_if<NumberExpr>(add->left.get());
                REQUIRE(addLeft != nullptr);
                CHECK(addLeft->value == doctest::Approx(2.0));
                auto* addRight = std::get_if<NumberExpr>(add->right.get());
                REQUIRE(addRight != nullptr);
                CHECK(addRight->value == doctest::Approx(3.0));
            }
        }
    }
}

SCENARIO("Parsing assignment")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"x = 5\"")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "x", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Number, "5", 5},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces AssignExpr with name \"x\" and NumberExpr value 5")
            {
                REQUIRE(result.has_value());
                auto* assign = std::get_if<AssignExpr>(result->get());
                REQUIRE(assign != nullptr);
                CHECK(assign->name == "x");
                auto* val = std::get_if<NumberExpr>(assign->value.get());
                REQUIRE(val != nullptr);
                CHECK(val->value == doctest::Approx(5.0));
            }
        }
        WHEN("parsing \"y = 2 + 3\"")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "y", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "3", 3},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces AssignExpr with name \"y\" and Add(2, 3)")
            {
                REQUIRE(result.has_value());
                auto* assign = std::get_if<AssignExpr>(result->get());
                REQUIRE(assign != nullptr);
                CHECK(assign->name == "y");
                auto* add = std::get_if<BinaryExpr>(assign->value.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                auto* addLeft = std::get_if<NumberExpr>(add->left.get());
                REQUIRE(addLeft != nullptr);
                CHECK(addLeft->value == doctest::Approx(2.0));
                auto* addRight = std::get_if<NumberExpr>(add->right.get());
                REQUIRE(addRight != nullptr);
                CHECK(addRight->value == doctest::Approx(3.0));
            }
        }
    }
}

SCENARIO("Parsing function calls")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"sqrt(4)\"")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "sqrt", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "4", 4},
                {TokenType::RParen, ")", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces CallExpr with name \"sqrt\" and one arg")
            {
                REQUIRE(result.has_value());
                auto* call = std::get_if<CallExpr>(result->get());
                REQUIRE(call != nullptr);
                CHECK(call->name == "sqrt");
                REQUIRE(call->args.size() == 1);
                auto* arg = std::get_if<NumberExpr>(call->args[0].get());
                REQUIRE(arg != nullptr);
                CHECK(arg->value == doctest::Approx(4.0));
            }
        }
        WHEN("parsing \"max(1, 2)\"")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "max", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "1", 1},
                {TokenType::Comma, ",", 0},
                {TokenType::Number, "2", 2},
                {TokenType::RParen, ")", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces CallExpr with name \"max\" and two args")
            {
                REQUIRE(result.has_value());
                auto* call = std::get_if<CallExpr>(result->get());
                REQUIRE(call != nullptr);
                CHECK(call->name == "max");
                REQUIRE(call->args.size() == 2);
                auto* arg0 = std::get_if<NumberExpr>(call->args[0].get());
                REQUIRE(arg0 != nullptr);
                CHECK(arg0->value == doctest::Approx(1.0));
                auto* arg1 = std::get_if<NumberExpr>(call->args[1].get());
                REQUIRE(arg1 != nullptr);
                CHECK(arg1->value == doctest::Approx(2.0));
            }
        }
    }
}

SCENARIO("Parsing errors")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("parsing \"2 +\"")
        {
            std::vector<Token> tokens{
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("returns ParseError (unexpected end)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("parsing \"(2 + 3\"")
        {
            std::vector<Token> tokens{
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "3", 3},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("returns ParseError (missing closing paren)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Parser contract: parse rejects empty token span")
{
    arc::test::Graph<node::Parser> graph;
    auto parser = graph.node.asTrait(trait::parser);

    GIVEN("a Parser node")
    {
        WHEN("calling parse with an empty token list")
        {
            std::vector<Token> empty;

            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(parser.parse(empty, std::string_view{}), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Parsing single-parameter function definitions")
{
    GIVEN("a Parser node")
    {
        arc::test::Graph<node::Parser> graph;
        auto parser = graph.node.asTrait(trait::parser);

        WHEN("parsing tokens for `f(x) = x + 1`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "1", 1},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr with name \"f\", params [\"x\"], body Add(Var(\"x\"), Num(1))")
            {
                REQUIRE(result.has_value());
                auto* funcDef = std::get_if<FuncDefExpr>(result->get());
                REQUIRE(funcDef != nullptr);
                CHECK(funcDef->name == "f");
                REQUIRE(funcDef->params.size() == 1);
                CHECK(funcDef->params[0] == "x");
                auto* add = std::get_if<BinaryExpr>(funcDef->body.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                auto* left = std::get_if<VariableExpr>(add->left.get());
                REQUIRE(left != nullptr);
                CHECK(left->name == "x");
                auto* right = std::get_if<NumberExpr>(add->right.get());
                REQUIRE(right != nullptr);
                CHECK(right->value == doctest::Approx(1.0));
            }
        }
        WHEN("parsing tokens for `sq(n) = n ^ 2`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "sq", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "n", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "n", 0},
                {TokenType::Caret, "^", 0},
                {TokenType::Number, "2", 2},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr with name \"sq\", params [\"n\"], body Pow(Var(\"n\"), Num(2))")
            {
                REQUIRE(result.has_value());
                auto* funcDef = std::get_if<FuncDefExpr>(result->get());
                REQUIRE(funcDef != nullptr);
                CHECK(funcDef->name == "sq");
                REQUIRE(funcDef->params.size() == 1);
                CHECK(funcDef->params[0] == "n");
                auto* pow = std::get_if<BinaryExpr>(funcDef->body.get());
                REQUIRE(pow != nullptr);
                CHECK(pow->op == BinaryOp::Pow);
                auto* left = std::get_if<VariableExpr>(pow->left.get());
                REQUIRE(left != nullptr);
                CHECK(left->name == "n");
                auto* right = std::get_if<NumberExpr>(pow->right.get());
                REQUIRE(right != nullptr);
                CHECK(right->value == doctest::Approx(2.0));
            }
        }
    }
}

SCENARIO("Parsing multi-parameter function definitions")
{
    GIVEN("a Parser node")
    {
        arc::test::Graph<node::Parser> graph;
        auto parser = graph.node.asTrait(trait::parser);

        WHEN("parsing tokens for `g(x, y) = x + y`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "g", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Comma, ",", 0},
                {TokenType::Identifier, "y", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Identifier, "y", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr with name \"g\", params [\"x\", \"y\"], body Add(Var(\"x\"), Var(\"y\"))")
            {
                REQUIRE(result.has_value());
                auto* funcDef = std::get_if<FuncDefExpr>(result->get());
                REQUIRE(funcDef != nullptr);
                CHECK(funcDef->name == "g");
                REQUIRE(funcDef->params.size() == 2);
                CHECK(funcDef->params[0] == "x");
                CHECK(funcDef->params[1] == "y");
                auto* add = std::get_if<BinaryExpr>(funcDef->body.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
                auto* left = std::get_if<VariableExpr>(add->left.get());
                REQUIRE(left != nullptr);
                CHECK(left->name == "x");
                auto* right = std::get_if<VariableExpr>(add->right.get());
                REQUIRE(right != nullptr);
                CHECK(right->name == "y");
            }
        }
        WHEN("parsing tokens for `h(a, b, c) = a * b + c`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "h", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "a", 0},
                {TokenType::Comma, ",", 0},
                {TokenType::Identifier, "b", 0},
                {TokenType::Comma, ",", 0},
                {TokenType::Identifier, "c", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "a", 0},
                {TokenType::Star, "*", 0},
                {TokenType::Identifier, "b", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Identifier, "c", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr with name \"h\", params [\"a\", \"b\", \"c\"]")
            {
                REQUIRE(result.has_value());
                auto* funcDef = std::get_if<FuncDefExpr>(result->get());
                REQUIRE(funcDef != nullptr);
                CHECK(funcDef->name == "h");
                REQUIRE(funcDef->params.size() == 3);
                CHECK(funcDef->params[0] == "a");
                CHECK(funcDef->params[1] == "b");
                CHECK(funcDef->params[2] == "c");
                auto* add = std::get_if<BinaryExpr>(funcDef->body.get());
                REQUIRE(add != nullptr);
                CHECK(add->op == BinaryOp::Add);
            }
        }
    }
}

SCENARIO("Function definition with complex body")
{
    GIVEN("a Parser node")
    {
        arc::test::Graph<node::Parser> graph;
        auto parser = graph.node.asTrait(trait::parser);

        WHEN("parsing tokens for `f(x) = x ^ 2 + 2 * x + 1`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Caret, "^", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "2", 2},
                {TokenType::Star, "*", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "1", 1},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr with correct nested expression body")
            {
                REQUIRE(result.has_value());
                auto* funcDef = std::get_if<FuncDefExpr>(result->get());
                REQUIRE(funcDef != nullptr);
                CHECK(funcDef->name == "f");
                REQUIRE(funcDef->params.size() == 1);
                CHECK(funcDef->params[0] == "x");
                auto* add1 = std::get_if<BinaryExpr>(funcDef->body.get());
                REQUIRE(add1 != nullptr);
                CHECK(add1->op == BinaryOp::Add);
            }
        }
    }
}

SCENARIO("Function definition vs variable assignment disambiguation")
{
    GIVEN("a Parser node")
    {
        arc::test::Graph<node::Parser> graph;
        auto parser = graph.node.asTrait(trait::parser);

        WHEN("parsing tokens for `x = 5`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "x", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Number, "5", 5},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces an AssignExpr (not FuncDefExpr)")
            {
                REQUIRE(result.has_value());
                CHECK(std::holds_alternative<AssignExpr>(**result));
            }
        }
        WHEN("parsing tokens for `f(x) = x + 1`")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "1", 1},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a FuncDefExpr (not AssignExpr or CallExpr)")
            {
                REQUIRE(result.has_value());
                CHECK(std::holds_alternative<FuncDefExpr>(**result));
            }
        }
    }
}

SCENARIO("Malformed function definitions produce errors")
{
    GIVEN("a Parser node")
    {
        arc::test::Graph<node::Parser> graph;
        auto parser = graph.node.asTrait(trait::parser);

        WHEN("parsing tokens for `f(1) = x` (number as parameter)")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Number, "1", 1},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("does not produce a FuncDefExpr (falls through to expression parsing, which errors)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("parsing tokens for `f(x,) = x` (trailing comma)")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Comma, ",", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("produces a ParseError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("parsing tokens for `f(,x) = x` (leading comma)")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Comma, ",", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::RParen, ")", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("does not produce a FuncDefExpr (falls through to expression parsing)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        WHEN("parsing tokens for `f(x = x + 1` (missing closing paren)")
        {
            std::vector<Token> tokens{
                {TokenType::Identifier, "f", 0},
                {TokenType::LParen, "(", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Equals, "=", 0},
                {TokenType::Identifier, "x", 0},
                {TokenType::Plus, "+", 0},
                {TokenType::Number, "1", 1},
                {TokenType::End, "", 0},
            };
            auto result = parser.parse(tokens, std::string_view{});

            THEN("does not produce a FuncDefExpr")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}
