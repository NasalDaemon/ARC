#include <doctest/doctest.h>

import examples.calculator.formatter;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("Formatting results")
{
    arc::test::Graph<node::Formatter> graph;
    auto formatter = graph.node.asTrait(trait::formatter);

    GIVEN("a Formatter node")
    {
        WHEN("formatting result 42.0")
        {
            auto result = formatter.formatResult(42.0);

            THEN("returns \"42\" (no unnecessary decimal)")
            {
                CHECK(result == "42");
            }
        }
        AND_WHEN("formatting result 3.14")
        {
            auto result = formatter.formatResult(3.14);

            THEN("returns \"3.14\"")
            {
                CHECK(result == "3.14");
            }
        }
    }
}

SCENARIO("Formatting errors")
{
    arc::test::Graph<node::Formatter> graph;
    auto formatter = graph.node.asTrait(trait::formatter);

    GIVEN("a Formatter node")
    {
        WHEN("formatting error \"undefined variable: x\"")
        {
            auto result = formatter.formatError("undefined variable: x");

            THEN("returns \"Error: undefined variable: x\"")
            {
                CHECK(result == "Error: undefined variable: x");
            }
        }
    }
}

SCENARIO("Formatting assignments")
{
    arc::test::Graph<node::Formatter> graph;
    auto formatter = graph.node.asTrait(trait::formatter);

    GIVEN("a Formatter node")
    {
        WHEN("formatting assignment \"x\" = 5")
        {
            auto result = formatter.formatAssignment("x", 5);

            THEN("returns \"x = 5\"")
            {
                CHECK(result == "x = 5");
            }
        }
    }
}

SCENARIO("Formatting variable listing")
{
    arc::test::Graph<node::Formatter> graph;
    auto formatter = graph.node.asTrait(trait::formatter);

    GIVEN("a Formatter node")
    {
        WHEN("formatting variables")
        {
            std::vector<std::pair<std::string, double>> vars{
                {"ans", 42}, {"x", 5}
            };
            auto result = formatter.formatVariables(vars);

            THEN("returns multi-line listing")
            {
                CHECK(result == "  ans = 42\n  x = 5");
            }
        }
    }
}

SCENARIO("Formatting function listing")
{
    arc::test::Graph<node::Formatter> graph;
    auto formatter = graph.node.asTrait(trait::formatter);

    GIVEN("a Formatter node")
    {
        WHEN("formatting functions")
        {
            std::vector<std::string> names{"abs", "add", "sqrt"};
            auto result = formatter.formatFunctions(names);

            THEN("returns readable list")
            {
                CHECK(result == "abs, add, sqrt");
            }
        }
    }
}
