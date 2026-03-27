#include <doctest/doctest.h>

import examples.calculator.evaluator;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

namespace {

auto makeNum(double v) -> ExprPtr
{
    return std::make_unique<Expression>(NumberExpr{v});
}

auto makeVar(std::string name) -> ExprPtr
{
    return std::make_unique<Expression>(VariableExpr{std::move(name)});
}

auto makeUnary(UnaryOp op, ExprPtr operand) -> ExprPtr
{
    return std::make_unique<Expression>(UnaryExpr{op, std::move(operand)});
}

auto makeBinary(BinaryOp op, ExprPtr left, ExprPtr right) -> ExprPtr
{
    return std::make_unique<Expression>(BinaryExpr{op, std::move(left), std::move(right)});
}

auto makeAssign(std::string name, ExprPtr value) -> ExprPtr
{
    return std::make_unique<Expression>(AssignExpr{std::move(name), std::move(value)});
}

auto makeCall(std::string name, std::vector<ExprPtr> args) -> ExprPtr
{
    return std::make_unique<Expression>(CallExpr{std::move(name), std::move(args)});
}

} // namespace

SCENARIO("Evaluating numeric literals")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node with mocked dependencies")
    {
        WHEN("evaluating NumberExpr(42)")
        {
            auto result = evaluator.evaluate(*makeNum(42));

            THEN("returns 42")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Evaluating binary operations")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        WHEN("evaluating Add(2, 3)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Add, makeNum(2), makeNum(3)));

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
            }
        }
        AND_WHEN("evaluating Sub(10, 3)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Sub, makeNum(10), makeNum(3)));

            THEN("returns 7")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(7.0));
            }
        }
        AND_WHEN("evaluating Mul(4, 5)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Mul, makeNum(4), makeNum(5)));

            THEN("returns 20")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(20.0));
            }
        }
        AND_WHEN("evaluating Div(10, 4)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Div, makeNum(10), makeNum(4)));

            THEN("returns 2.5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(2.5));
            }
        }
        AND_WHEN("evaluating Pow(2, 10)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Pow, makeNum(2), makeNum(10)));

            THEN("returns 1024")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(1024.0));
            }
        }
    }
}

SCENARIO("Evaluating division by zero")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        WHEN("evaluating Div(1, 0)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Div, makeNum(1), makeNum(0)));

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Evaluating unary negation")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        WHEN("evaluating Negate(5)")
        {
            auto result = evaluator.evaluate(*makeUnary(UnaryOp::Negate, makeNum(5)));

            THEN("returns -5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(-5.0));
            }
        }
    }
}

SCENARIO("Evaluating variable references")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        graph.mocks->define(
            [](trait::Variables::get, std::string_view name) -> std::optional<double> {
                if (name == "x") return 42.0;
                return std::nullopt;
            }
        );

        WHEN("evaluating VariableExpr(\"x\") with mock returning 42")
        {
            auto result = evaluator.evaluate(*makeVar("x"));

            THEN("returns 42")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Evaluating undefined variable")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node with mock returning nullopt")
    {
        graph.mocks->define(
            [](trait::Variables::get, std::string_view) -> std::optional<double> {
                return std::nullopt;
            }
        );

        WHEN("evaluating VariableExpr(\"y\")")
        {
            auto result = evaluator.evaluate(*makeVar("y"));

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Evaluating assignment")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    graph.mocks->enableCallCounting();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        WHEN("evaluating AssignExpr(\"x\", NumberExpr(5))")
        {
            auto result = evaluator.evaluate(*makeAssign("x", makeNum(5)));

            THEN("mock Variables::set is called and returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
                CHECK(graph.mocks->methodCallCount<trait::Variables::set>() == 1);
            }
        }
    }
}

SCENARIO("Evaluating function calls")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node with mock Functions")
    {
        graph.mocks->define(
            [](trait::Functions::call, std::string_view name, std::span<double const> args)
                -> std::expected<double, EvalError>
            {
                if (name == "sqrt" && args.size() == 1)
                    return 2.0;
                return std::unexpected(EvalError{"unknown"});
            }
        );

        WHEN("evaluating CallExpr(\"sqrt\", [NumberExpr(4)])")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(4));
            auto result = evaluator.evaluate(*makeCall("sqrt", std::move(args)));

            THEN("returns 2")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(2.0));
            }
        }
    }
}

SCENARIO("Evaluating nested expressions")
{
    arc::test::Graph<node::Evaluator> graph;
    graph.mocks->setReturnDefault();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node")
    {
        WHEN("evaluating Add(Mul(2, 3), Pow(2, 3))")
        {
            auto result = evaluator.evaluate(*makeBinary(
                BinaryOp::Add,
                makeBinary(BinaryOp::Mul, makeNum(2), makeNum(3)),
                makeBinary(BinaryOp::Pow, makeNum(2), makeNum(3))
            ));

            THEN("returns 14")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(14.0));
            }
        }
    }
}
