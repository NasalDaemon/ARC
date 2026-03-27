#include <doctest/doctest.h>

import examples.calculator.functions;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("Calling built-in unary functions")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("calling \"abs\" with [-5]")
        {
            std::array args{-5.0};
            auto result = functions.call("abs", args);

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 5.0);
            }
        }
        AND_WHEN("calling \"sqrt\" with [16]")
        {
            std::array args{16.0};
            auto result = functions.call("sqrt", args);

            THEN("returns 4")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 4.0);
            }
        }
        AND_WHEN("calling \"neg\" with [3]")
        {
            std::array args{3.0};
            auto result = functions.call("neg", args);

            THEN("returns -3")
            {
                REQUIRE(result.has_value());
                CHECK(*result == -3.0);
            }
        }
    }
}

SCENARIO("Calling built-in binary functions")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("calling \"add\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("add", args);

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 5.0);
            }
        }
        AND_WHEN("calling \"max\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("max", args);

            THEN("returns 3")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 3.0);
            }
        }
        AND_WHEN("calling \"min\" with [2, 3]")
        {
            std::array args{2.0, 3.0};
            auto result = functions.call("min", args);

            THEN("returns 2")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 2.0);
            }
        }
        AND_WHEN("calling \"pow\" with [2, 10]")
        {
            std::array args{2.0, 10.0};
            auto result = functions.call("pow", args);

            THEN("returns 1024")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 1024.0);
            }
        }
    }
}

SCENARIO("Calling trig functions")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("calling \"sin\" with [0]")
        {
            std::array args{0.0};
            auto result = functions.call("sin", args);

            THEN("returns 0")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 0.0);
            }
        }
        AND_WHEN("calling \"cos\" with [0]")
        {
            std::array args{0.0};
            auto result = functions.call("cos", args);

            THEN("returns 1")
            {
                REQUIRE(result.has_value());
                CHECK(*result == 1.0);
            }
        }
    }
}

SCENARIO("Wrong argument count")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("calling \"sqrt\" with [1, 2]")
        {
            std::array args{1.0, 2.0};
            auto result = functions.call("sqrt", args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        AND_WHEN("calling \"add\" with [1]")
        {
            std::array args{1.0};
            auto result = functions.call("add", args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Unknown function")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("calling \"unknown\" with [1]")
        {
            std::array args{1.0};
            auto result = functions.call("unknown", args);

            THEN("returns EvalError")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Listing functions")
{
    arc::test::Graph<node::Functions> graph;
    auto functions = graph.node.asTrait(trait::functions);

    GIVEN("a Functions node")
    {
        WHEN("listing functions")
        {
            auto result = functions.list();

            THEN("returns all built-in function names")
            {
                CHECK_FALSE(result.empty());
                CHECK(std::ranges::find(result, "abs") != result.end());
                CHECK(std::ranges::find(result, "sqrt") != result.end());
                CHECK(std::ranges::find(result, "neg") != result.end());
                CHECK(std::ranges::find(result, "sin") != result.end());
                CHECK(std::ranges::find(result, "cos") != result.end());
                CHECK(std::ranges::find(result, "tan") != result.end());
                CHECK(std::ranges::find(result, "log") != result.end());
                CHECK(std::ranges::find(result, "ln") != result.end());
                CHECK(std::ranges::find(result, "add") != result.end());
                CHECK(std::ranges::find(result, "sub") != result.end());
                CHECK(std::ranges::find(result, "mul") != result.end());
                CHECK(std::ranges::find(result, "div") != result.end());
                CHECK(std::ranges::find(result, "pow") != result.end());
                CHECK(std::ranges::find(result, "min") != result.end());
                CHECK(std::ranges::find(result, "max") != result.end());
            }
        }
    }
}
