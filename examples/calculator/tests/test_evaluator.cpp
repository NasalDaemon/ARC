import examples.calculator.node.evaluator;
import examples.calculator.types;
import examples.calculator.traits;
import examples.calculator.tests.graphs;
import arc;
import std;

#include "arc/doctest.h"

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
    GIVEN("an Evaluator node with mocked dependencies")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);
        WHEN("evaluating NumberExpr(42)")
        {
            auto result = evaluator.evaluate(*makeNum(42));

            THEN("returns 42")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Evaluating binary operations")
{
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);
        WHEN("evaluating Add(2, 3)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Add, makeNum(2), makeNum(3)));

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(5.0));
            }
        }
        WHEN("evaluating Sub(10, 3)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Sub, makeNum(10), makeNum(3)));

            THEN("returns 7")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(7.0));
            }
        }
        WHEN("evaluating Mul(4, 5)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Mul, makeNum(4), makeNum(5)));

            THEN("returns 20")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(20.0));
            }
        }
        WHEN("evaluating Div(10, 4)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Div, makeNum(10), makeNum(4)));

            THEN("returns 2.5")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(2.5));
            }
        }
        WHEN("evaluating Pow(2, 10)")
        {
            auto result = evaluator.evaluate(*makeBinary(BinaryOp::Pow, makeNum(2), makeNum(10)));

            THEN("returns 1024")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(1024.0));
            }
        }
    }
}

SCENARIO("Evaluating division by zero")
{
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        auto evaluator = graph.asTrait(trait::evaluator);
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
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);
        WHEN("evaluating Negate(5)")
        {
            auto result = evaluator.evaluate(*makeUnary(UnaryOp::Negate, makeNum(5)));

            THEN("returns -5")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(-5.0));
            }
        }
    }
}

SCENARIO("Evaluating variable references")
{
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);

        graph.mocks->define(
            [](Variables::get, std::string_view name) -> std::optional<double> {
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
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Evaluating undefined variable")
{
    GIVEN("an Evaluator node with mock returning nullopt")
    {
        arc::test::Graph<node::Evaluator> graph;
        auto evaluator = graph.asTrait(trait::evaluator);

        graph.mocks->methodReturns<Variables::get>(std::optional<double>{});

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
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);
        WHEN("evaluating AssignExpr(\"x\", NumberExpr(5))")
        {
            auto result = evaluator.evaluate(*makeAssign("x", makeNum(5)));

            THEN("Variables::set is called for the variable and for \"ans\", and returns AssignResult")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<AssignResult>(*result));
                CHECK(std::get<AssignResult>(*result).value == doctest::Approx(5.0));
                CHECK(std::get<AssignResult>(*result).name == "x");
                CHECK(graph.mocks->methodCallCount<Variables::set>() == 2);
            }
        }
    }
}

SCENARIO("Evaluating numeric expressions sets \"ans\"")
{
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        auto evaluator = graph.asTrait(trait::evaluator);

        WHEN("evaluating NumberExpr(42)")
        {
            evaluator.evaluate(*makeNum(42));

            THEN("Variables::set is called with \"ans\" and 42.0")
            {
                auto visitor = graph.mocks->visitCallLogs<Variables::set, std::string, double>();
                REQUIRE(visitor.size() >= 1);
                auto ansCall = visitor.findNext(std::tuple{"ans", 42.0});
                REQUIRE(ansCall.has_value());
            }
        }
    }
}

SCENARIO("Evaluating a function definition")
{
    GIVEN("an Evaluator node with mocked Functions")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        WHEN("evaluating a FuncDefExpr for \"f(x) = x + 1\"")
        {
            auto result = evaluator.evaluate(*makeFuncDef(
                "f",
                {"x"},
                makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1))
            ));

            THEN("Functions::define is called once and returns FuncDefResult with the function name")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<FuncDefResult>(*result));
                CHECK(std::get<FuncDefResult>(*result).name == "f");
                CHECK(graph.mocks->methodCallCount<Functions::define>() == 1);
            }
            THEN("Variables::set is not called — function definitions do not update \"ans\"")
            {
                CHECK(graph.mocks->methodCallCount<Variables::set>() == 0);
            }
        }
        WHEN("Functions::define returns an error")
        {
            graph.mocks->methodReturns<Functions::define>(
                std::expected<void, EvalError>{std::unexpect, EvalError{"cannot shadow builtin"}});

            auto result = evaluator.evaluate(*makeFuncDef("abs", {"x"}, makeNum(1)));

            THEN("the error is propagated")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message.contains("cannot shadow builtin"));
            }
        }
    }
}

SCENARIO("Evaluating builtin function calls")
{
    GIVEN("an Evaluator node with mock Functions returning a value for \"sqrt\"")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);

        graph.mocks->define(
            [](Functions::call, std::string_view name, std::span<double const> args)
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
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(2.0));
            }
        }
    }
}

SCENARIO("Evaluating nested expressions")
{
    GIVEN("an Evaluator node")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);
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
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(14.0));
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

        UserFunction userFunc{
            {"x"},
            makeBinary(BinaryOp::Add, makeVar("x"), makeNum(1)),
            ""
        };

        tests::MockVariableStore varStore;
        varStore.install(graph);

        graph.mocks->methodReturns<Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        graph.mocks->define(
            [&userFunc](Functions::get, std::string_view name, std::size_t arity)
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
                CHECK(graph.mocks->methodCallCount<Variables::set>() >= 1);
            }
            THEN("the body is evaluated and result is 6.0")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(6.0));
            }
            THEN("Variables are restored (previous x value removed)")
            {
                CHECK(graph.mocks->methodCallCount<Variables::remove>() >= 1);
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

        UserFunction funcArity1{
            {"x"},
            makeBinary(BinaryOp::Mul, makeVar("x"), makeNum(2)),
            ""
        };
        UserFunction funcArity2{
            {"x", "y"},
            makeBinary(BinaryOp::Add, makeVar("x"), makeVar("y")),
            ""
        };

        tests::MockVariableStore varStore;
        varStore.install(graph);

        graph.mocks->methodReturns<Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        graph.mocks->define(
            [&funcArity1, &funcArity2](Functions::get, std::string_view name, std::size_t arity)
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
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(6.0));
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
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(7.0));
            }
        }
    }
}

SCENARIO("Evaluator binds and restores variables when calling a user function")
{
    GIVEN(R"(an Evaluator with f(x) = x * 2 and a pre-existing variable x = 99)")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        auto body = makeBinary(BinaryOp::Mul, makeVar("x"), makeNum(2));
        UserFunction userFunc{{"x"}, body->clone(), ""};

        tests::MockVariableStore varStore;
        varStore.vars["x"] = 99.0;
        varStore.install(graph);

        graph.mocks->methodReturns<Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        graph.mocks->define(
            [&userFunc](Functions::get, std::string_view name, std::size_t arity) -> UserFunction const*
            {
                return (name == "f" && arity == 1) ? &userFunc : nullptr;
            });

        WHEN("evaluating f(7)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(7));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("body evaluates with x = 7, giving 14; and x is restored to 99 afterward")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(14.0));
                CHECK(varStore.vars.at("x") == doctest::Approx(99.0));
            }
        }
    }
}

SCENARIO("Unknown function returns EvalError with name/arity")
{
    GIVEN("an Evaluator where no builtin or user function matches f/2")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        auto evaluator = graph.asTrait(trait::evaluator);

        graph.mocks->methodReturns<BuiltinFunctions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function: f/2"});

        WHEN("evaluating CallExpr for f(1, 2)")
        {
            std::vector<ExprPtr> args;
            args.push_back(makeNum(1));
            args.push_back(makeNum(2));
            auto result = evaluator.evaluate(*makeCall("f", std::move(args)));

            THEN("returns EvalError with \"unknown function: f/2\"")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().message == "unknown function: f/2");
            }
        }
    }
}

SCENARIO("Variable scoping: pre-existing variable is restored via set, not remove")
{
    GIVEN(R"(an Evaluator with f(x) = x and a pre-existing variable x = 10)")
    {
        arc::test::Graph<node::Evaluator> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto evaluator = graph.asTrait(trait::evaluator);

        UserFunction userFunc{
            {"x"},
            makeVar("x"),
            ""
        };

        graph.mocks->methodReturns<Functions::call>(
            std::expected<double, EvalError>{std::unexpect, "unknown function"});

        graph.mocks->define(
            [&userFunc](Functions::get, std::string_view name, std::size_t arity)
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
                CHECK(graph.mocks->methodCallCount<Variables::set>() >= 1);
            }
            THEN("result is 5.0")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::holds_alternative<NumberResult>(*result));
                CHECK(std::get<NumberResult>(*result).value == doctest::Approx(5.0));
            }
            THEN("Variables.set(\"x\", 10.0) is called (restore)")
            {
                CHECK(graph.mocks->methodCallCount<Variables::set>() >= 2);
            }
        }
    }
}
