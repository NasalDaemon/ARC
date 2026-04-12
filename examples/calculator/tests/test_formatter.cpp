#include <doctest/doctest.h>

import examples.calculator.node.formatter;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;
using namespace std::string_view_literals;

SCENARIO("Formatting results")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("formatting result 42.0")
        {
            auto result = formatter.formatResult(42.0);

            THEN("returns \"42\" (no unnecessary decimal)")
            {
                CHECK(result == "42");
            }
        }
        WHEN("formatting result 3.14")
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
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("formatting error \"undefined variable: x\"")
        {
            auto result = formatter.formatError("undefined variable: x"sv);

            THEN("returns \"Error: undefined variable: x\"")
            {
                CHECK(result == "Error: undefined variable: x");
            }
        }
    }
}

SCENARIO("Formatting assignments")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("formatting assignment \"x\" = 5")
        {
            auto result = formatter.formatAssignment("x"sv, 5);

            THEN("returns \"x = 5\"")
            {
                CHECK(result == "x = 5");
            }
        }
    }
}

SCENARIO("Formatting variable listing")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
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
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("formatting functions")
        {
            std::vector<std::string> builtins{"abs", "add", "sqrt"};
            std::vector<std::pair<std::string, UserFunction const*>> noUserFuncs;
            auto result = formatter.formatFunctions(builtins, noUserFuncs);

            THEN("returns readable list")
            {
                CHECK(result.contains("abs"));
                CHECK(result.contains("add"));
                CHECK(result.contains("sqrt"));
            }
        }
    }
}

SCENARIO("Formatter contract: pre-contract violations")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("calling formatError with empty message")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(formatter.formatError(""sv), arc::ContractViolation);
            }
        }
        WHEN("calling formatAssignment with empty name")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(formatter.formatAssignment(""sv, 42.0), arc::ContractViolation);
            }
        }
        WHEN("calling formatVariables with empty span")
        {
            std::vector<std::pair<std::string, double>> empty;

            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(formatter.formatVariables(empty), arc::ContractViolation);
            }
        }
        WHEN("calling formatFunctions with empty builtins and empty userFuncs")
        {
            std::vector<std::string> emptyBuiltins;
            std::vector<std::pair<std::string, UserFunction const*>> emptyUserFuncs;

            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(formatter.formatFunctions(emptyBuiltins, emptyUserFuncs), arc::ContractViolation);
            }
        }
    }
}

namespace {

struct BuggyFormatter : arc::NodeImpl<Formatter>
{
    std::string formatResult(double) const { return ""; }
    std::string formatError(std::string_view) const { return "err"; }
    std::string formatAssignment(std::string_view name, double) const { return std::string(name); }
    std::string formatVariables(std::span<std::pair<std::string, double> const>) const { return "vars"; }
    std::string formatFunctions(std::span<std::string const>, std::span<std::pair<std::string, UserFunction const*> const>) const { return "fns"; }
    std::string formatFunctionDef(std::string_view name, std::span<std::string const>) const { return std::string(name); }
};

} // namespace

SCENARIO("Formatter contract: formatResult post-contract fires when result is empty")
{
    GIVEN("a buggy Formatter that returns empty string from formatResult")
    {
        arc::test::Graph<BuggyFormatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("calling formatResult")
        {
            THEN("triggers a post-contract violation")
            {
                CHECK_THROWS_AS(formatter.formatResult(42.0), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Formatting a function definition confirmation")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("formatting function def with name=\"f\", params=[\"x\"]")
        {
            std::vector<std::string> params{"x"};
            auto result = formatter.formatFunctionDef("f"sv, params);

            THEN("returns formatted definition string")
            {
                CHECK(result == "f(x) defined");
            }
        }
        WHEN("formatting with name=\"g\", params=[\"x\", \"y\"]")
        {
            std::vector<std::string> params{"x", "y"};
            auto result = formatter.formatFunctionDef("g"sv, params);

            THEN("returns formatted definition string")
            {
                CHECK(result == "g(x, y) defined");
            }
        }
    }
}

SCENARIO("Formatter contract: formatFunctionDef rejects empty name")
{
    GIVEN("a Formatter node")
    {
        arc::test::Graph<node::Formatter> graph;
        auto formatter = graph.node.asTrait(trait::formatter);
        WHEN("calling formatFunctionDef with empty name")
        {
            std::vector<std::string> params{"x"};

            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(formatter.formatFunctionDef(""sv, params), arc::ContractViolation);
            }
        }
    }
}
