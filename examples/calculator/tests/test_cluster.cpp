#include <doctest/doctest.h>

import examples.calculator.clusters;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

namespace {

// Helper: run a full expression through the pipeline
auto eval(arc::Graph<domain::Calculator>& graph, std::string_view input)
    -> std::expected<double, std::string>
{
    auto tokeniser = graph.tokeniser.asTrait(trait::tokeniser);
    auto parser = graph.parser.asTrait(trait::parser);
    auto evaluator = graph.evaluator.asTrait(trait::evaluator);

    auto tokens = tokeniser.tokenise(input);
    if (!tokens) return std::unexpected(tokens.error().message);

    auto expr = parser.parse(*tokens);
    if (!expr) return std::unexpected(expr.error().message);

    auto result = evaluator.evaluate(**expr);
    if (!result) return std::unexpected(result.error().message);

    return *result;
}

} // namespace

SCENARIO("Basic arithmetic through full pipeline")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"2 + 3\"")
        {
            auto result = eval(graph, "2 + 3");

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
            }
        }
        AND_WHEN("evaluating \"10 - 3 * 2\"")
        {
            auto result = eval(graph, "10 - 3 * 2");

            THEN("returns 4")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(4.0));
            }
        }
        AND_WHEN("evaluating \"(10 - 3) * 2\"")
        {
            auto result = eval(graph, "(10 - 3) * 2");

            THEN("returns 14")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(14.0));
            }
        }
    }
}

SCENARIO("BIDMAS precedence end-to-end")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"2 + 3 * 4\"")
        {
            auto result = eval(graph, "2 + 3 * 4");

            THEN("returns 14")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(14.0));
            }
        }
        AND_WHEN("evaluating \"2 * 3 + 4 * 5\"")
        {
            auto result = eval(graph, "2 * 3 + 4 * 5");

            THEN("returns 26")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(26.0));
            }
        }
        AND_WHEN("evaluating \"2 ^ 3 + 1\"")
        {
            auto result = eval(graph, "2 ^ 3 + 1");

            THEN("returns 9")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(9.0));
            }
        }
    }
}

SCENARIO("Variable assignment and recall")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"x = 5\"")
        {
            auto result = eval(graph, "x = 5");

            THEN("returns 5 and variables contains x = 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
                CHECK(graph.Variables.asTrait(trait::variables).get("x") == std::optional<double>{5.0});
            }
            AND_WHEN("evaluating \"x * 2 + 1\"")
            {
                auto result2 = eval(graph, "x * 2 + 1");

                THEN("returns 11")
                {
                    REQUIRE(result2.has_value());
                    CHECK(*result2 == doctest::Approx(11.0));
                }
            }
            AND_WHEN("evaluating \"y = x ^ 2\"")
            {
                auto result2 = eval(graph, "y = x ^ 2");

                THEN("returns 25 and variables contains y = 25")
                {
                    REQUIRE(result2.has_value());
                    CHECK(*result2 == doctest::Approx(25.0));
                    CHECK(graph.Variables.asTrait(trait::variables).get("y") == std::optional<double>{25.0});
                }
            }
        }
    }
}

SCENARIO("ans variable")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"2 + 3\" and storing result in ans")
        {
            auto result = eval(graph, "2 + 3");
            REQUIRE(result.has_value());
            graph.Variables.asTrait(trait::variables).set("ans", *result);

            AND_WHEN("evaluating \"ans * 10\"")
            {
                auto result2 = eval(graph, "ans * 10");

                THEN("returns 50")
                {
                    REQUIRE(result2.has_value());
                    CHECK(*result2 == doctest::Approx(50.0));
                }
            }
        }
    }
}

SCENARIO("Function calls end-to-end")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"sqrt(16)\"")
        {
            auto result = eval(graph, "sqrt(16)");

            THEN("returns 4")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(4.0));
            }
        }
        AND_WHEN("evaluating \"max(3, 7)\"")
        {
            auto result = eval(graph, "max(3, 7)");

            THEN("returns 7")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(7.0));
            }
        }
        AND_WHEN("evaluating \"abs(-42)\"")
        {
            auto result = eval(graph, "abs(-42)");

            THEN("returns 42")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(42.0));
            }
        }
    }
}

SCENARIO("Complex nested expressions")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"sqrt(add(9, 16))\"")
        {
            auto result = eval(graph, "sqrt(add(9, 16))");

            THEN("returns 5")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(5.0));
            }
        }
        AND_WHEN("evaluating \"max(2 ^ 3, 3 ^ 2)\"")
        {
            auto result = eval(graph, "max(2 ^ 3, 3 ^ 2)");

            THEN("returns 9")
            {
                REQUIRE(result.has_value());
                CHECK(*result == doctest::Approx(9.0));
            }
        }
    }
}

SCENARIO("Error propagation")
{
    arc::Graph<domain::Calculator> graph;

    GIVEN("a Calculator graph")
    {
        WHEN("evaluating \"1 / 0\"")
        {
            auto result = eval(graph, "1 / 0");

            THEN("returns error (division by zero)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        AND_WHEN("evaluating \"2 +\"")
        {
            auto result = eval(graph, "2 +");

            THEN("returns error (unexpected end)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        AND_WHEN("evaluating \"undefined_var\"")
        {
            auto result = eval(graph, "undefined_var");

            THEN("returns error (undefined variable)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
        AND_WHEN("evaluating \"unknown_fn(1)\"")
        {
            auto result = eval(graph, "unknown_fn(1)");

            THEN("returns error (unknown function)")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}
