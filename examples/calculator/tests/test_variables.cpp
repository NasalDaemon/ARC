#include <doctest/doctest.h>

import examples.calculator.variables;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;
using namespace std::string_literals;
using namespace std::string_view_literals;

SCENARIO("Variable storage and retrieval")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node")
    {
        WHEN("setting \"x\" to 42")
        {
            vars.set("x"s, 42);

            THEN("getting \"x\" returns 42")
            {
                auto result = vars.get("x"sv);
                REQUIRE(result.has_value());
                CHECK(*result == 42.0);
            }
        }
    }
}

SCENARIO("Undefined variable")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node")
    {
        WHEN("getting \"undefined_var\"")
        {
            auto result = vars.get("undefined_var"sv);

            THEN("returns nullopt")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Variable overwrite")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node with \"x\" = 1")
    {
        vars.set("x"s, 1);

        WHEN("setting \"x\" to 2")
        {
            vars.set("x"s, 2);

            THEN("getting \"x\" returns 2")
            {
                auto result = vars.get("x"sv);
                REQUIRE(result.has_value());
                CHECK(*result == 2.0);
            }
        }
    }
}

SCENARIO("Listing variables")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node with \"x\" = 1 and \"y\" = 2")
    {
        vars.set("x"s, 1);
        vars.set("y"s, 2);

        WHEN("listing variables")
        {
            auto result = vars.list();

            THEN("returns both pairs sorted by name")
            {
                REQUIRE(result.size() == 2);
                CHECK(result[0].first == "x");
                CHECK(result[0].second == 1.0);
                CHECK(result[1].first == "y");
                CHECK(result[1].second == 2.0);
            }
        }
    }
}

SCENARIO("Clearing all variables")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node with x=1 and y=2")
    {
        vars.set("x"s, 1);
        vars.set("y"s, 2);

        WHEN("calling clear()")
        {
            vars.clear();

            THEN("get(\"x\") returns nullopt")
            {
                CHECK_FALSE(vars.get("x"sv).has_value());
            }
            AND_THEN("get(\"y\") returns nullopt")
            {
                CHECK_FALSE(vars.get("y"sv).has_value());
            }
            AND_THEN("list() returns empty")
            {
                CHECK(vars.list().empty());
            }
        }
    }
}

SCENARIO("Variables contract: get/set reject empty name")
{
    arc::test::Graph<node::Variables> graph;
    auto vars = graph.node.asTrait(trait::variables);

    GIVEN("a Variables node")
    {
        WHEN("calling get with an empty name")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(vars.get(""sv), arc::ContractViolation);
            }
        }
        AND_WHEN("calling set with an empty name")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(vars.set(std::string{""}, 42.0), arc::ContractViolation);
            }
        }
    }
}
