#include <doctest/doctest.h>

import examples.calculator.evaluator;
import examples.calculator.types;
import examples.calculator.traits;
import examples.calculator.tests.graphs;
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

auto makeFuncDef(std::string name, std::vector<std::string> params, ExprPtr body, std::string source = "") -> ExprPtr
{
    return std::make_unique<Expression>(FuncDefExpr{std::move(name), std::move(params), std::move(body), std::move(source)});
}

} // namespace

SCENARIO("Evaluating numeric literals")
{
    auto graph = tests::makeEvaluatorGraph();
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
    auto graph = tests::makeEvaluatorGraph();
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
        WHEN("evaluating Sub(10, 3)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Sub, makeNum(10), makeNum(3)));

            THEN("returns 7")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(7.0));
            }
        }
        WHEN("evaluating Mul(4, 5)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Mul, makeNum(4), makeNum(5)));

            THEN("returns 20")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(20.0));
            }
        }
        WHEN("evaluating Div(10, 4)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Div, makeNum(10), makeNum(4)));

            THEN("returns 2.5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(2.5));
            }
        }
        WHEN("evaluating Pow(2, 10)")
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
    auto graph = tests::makeEvaluatorGraph();
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
    auto graph = tests::makeEvaluatorGraph();
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
    auto graph = tests::makeEvaluatorGraph();
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
    auto graph = tests::makeEvaluatorGraph();
    auto evaluator = graph.asTrait(trait::evaluator);

    GIVEN("an Evaluator node with mock returning nullopt")
    {
        graph.mocks->methodReturns<trait::Variables::get>(std::optional<double>{});

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
    auto graph = tests::makeEvaluatorGraph();
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
    auto graph = tests::makeEvaluatorGraph();
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

SCENARIO("Evaluating a function definition stores it")
{
    GIVEN("an Evaluator node with mocked Functions")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        // No need to mock define here, just use the default mock which returns success

        WHEN("evaluating a FuncDefExpr for \"f(x) = x + 1\"")
        {
            auto result = evaluator.evaluate(*makeFuncDef(
                "f",
                {"x"},
                makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1))
            ));

            THEN("mock Functions.define is called with name=\"f\", params=[\"x\"]")
            {
                CHECK(graph.mocks->methodCallCount<trait::Functions::define>() == 1);
            }
            THEN("returns a value (0.0)")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(0.0));
            }
        }
    }
}

SCENARIO("Calling a user-defined function")
{
    GIVEN("an Evaluator node with mocked Functions and Variables")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        // Create a user-defined function f(x) = x + 1
        UserFunction userFunc{
            {"x"},                              // params
            makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1)),  // body: x + 1
            ""                                  // source
        };

        tests::MockVariableStore varStore;
        varStore.install(graph);

        // Mock Functions::call to return error (no builtin "f")
        graph.mocks->methodReturns<trait::Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        // Mock Functions::getUserFunction to return our user function for f/1
        graph.mocks->define(
            [&userFunc](trait::Functions::getUserFunction, std::string_view name, std::size_t arity)
                -> UserFunction const*
            {
                if (name == "f" && arity == 1)
                    return &userFunc;
                return nullptr;
            }
        );

        WHEN("evaluating CallExpr for f(5)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(5));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("Variables.set is called to bind the parameter")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::set>() >= 1);
            }
            THEN("the body is evaluated and result is 6.0")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(6.0));
            }
            THEN("Variables are restored (previous x value removed)")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::remove>() >= 1);
            }
        }
    }
}

SCENARIO("Calling an overloaded user function selects correct arity")
{
    GIVEN("an Evaluator node with mocked Functions and Variables")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);

        // Create two overloaded user functions
        UserFunction funcArity1{
            {"x"},                                  // params
            makeBinary(BinaryOp::Mul, makeVar("x"), makeNum(2)),  // body: x * 2
            ""                                      // source
        };
        UserFunction funcArity2{
            {"x", "y"},                             // params
            makeBinary(BinaryOp::Add, makeVar("x"), makeVar("y")),  // body: x + y
            ""                                      // source
        };

        tests::MockVariableStore varStore;
        varStore.install(graph);

        // Mock Functions::call to return error (no builtin "f")
        graph.mocks->methodReturns<trait::Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        // Mock Functions::getUserFunction to return correct overload by arity
        graph.mocks->define(
            [&funcArity1, &funcArity2](trait::Functions::getUserFunction, std::string_view name, std::size_t arity)
                -> UserFunction const*
            {
                if (name == "f" && arity == 1)
                    return &funcArity1;
                if (name == "f" && arity == 2)
                    return &funcArity2;
                return nullptr;
            }
        );

        WHEN("evaluating CallExpr for f(3)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(3));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("result is 6.0 (uses f/1: x*2)")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(6.0));
            }
        }
        WHEN("evaluating CallExpr for f(3, 4)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(3));
            args.push_back(makeNum(4));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("result is 7.0 (uses f/2: x+y)")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(7.0));
            }
        }
    }
}

SCENARIO("User function with wrong arity (no matching overload)")
{
    GIVEN("an Evaluator node with mocked Functions")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);

        // Mock Functions::call to return "unknown function" error
        graph.mocks->define(
            [](trait::Functions::call, std::string_view name, std::span<double const> /*args*/)
                -> std::expected<double, EvalError>
            {
                return std::unexpected(EvalError{"unknown function: " + std::string(name)});
            }
        );

        // Mock Functions::getUserFunction to return nullptr (no matching arity)
        graph.mocks->methodReturns<trait::Functions::getUserFunction>(
            static_cast<UserFunction const*>(nullptr));

        WHEN("evaluating CallExpr for f(1, 2)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(1));
            args.push_back(makeNum(2));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("returns EvalError with \"unknown function\"")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Variable scoping in user function calls")
{
    GIVEN("an Evaluator with mocked Functions and Variables")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        // Create a user-defined function f(x) = x
        UserFunction userFunc{
            {"x"},                              // params
            makeVar("x"),                       // body: x (just return the param)
            ""                                  // source
        };

        graph.mocks->methodReturns<trait::Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        // Mock Functions::getUserFunction to return our user function for f/1
        graph.mocks->define(
            [&userFunc](trait::Functions::getUserFunction, std::string_view name, std::size_t arity)
                -> UserFunction const*
            {
                if (name == "f" && arity == 1)
                    return &userFunc;
                return nullptr;
            }
        );

        tests::MockVariableStore varStore;
        varStore.vars = {{"x", 10.0}};
        varStore.install(graph);

        WHEN("evaluating CallExpr for f(5)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(5));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("Variables.set(\"x\", 5.0) is called (binding)")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::set>() >= 1);
            }
            THEN("result is 5.0")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
            }
            THEN("Variables.set(\"x\", 10.0) is called (restore)")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::set>() >= 2);
            }
        }
    }
}
