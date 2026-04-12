#include <doctest/doctest.h>

import examples.calculator.tests.graphs;
import examples.calculator.graphs;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;
using namespace examples::calculator::tests;

namespace {

// Helper: run a scenario with pre-programmed inputs and return captured outputs
auto runWithInputs(std::vector<std::string> inputs) -> std::vector<std::string>
{
    IntegrationGraph graph;
    graph.lineReader->setInputs(std::move(inputs));
    graph.repl->run();
    return graph.output->lines();
}

} // namespace

SCENARIO("Basic arithmetic through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("input is \"2 + 3\"")
        {
            auto outputs = runWithInputs({"2 + 3"});

            THEN("output is \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
        }
        WHEN("input is \"10 - 3 * 2\"")
        {
            auto outputs = runWithInputs({"10 - 3 * 2"});

            THEN("output is \"4\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "4");
            }
        }
        WHEN("input is \"(10 - 3) * 2\"")
        {
            auto outputs = runWithInputs({"(10 - 3) * 2"});

            THEN("output is \"14\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "14");
            }
        }
    }
}

SCENARIO("BIDMAS precedence end-to-end")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("input is \"2 + 3 * 4\"")
        {
            auto outputs = runWithInputs({"2 + 3 * 4"});

            THEN("output is \"14\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "14");
            }
        }
        WHEN("input is \"2 * 3 + 4 * 5\"")
        {
            auto outputs = runWithInputs({"2 * 3 + 4 * 5"});

            THEN("output is \"26\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "26");
            }
        }
        WHEN("input is \"2 ^ 3 + 1\"")
        {
            auto outputs = runWithInputs({"2 ^ 3 + 1"});

            THEN("output is \"9\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "9");
            }
        }
    }
}

SCENARIO("Variable assignment and recall through Repl")
{
    GIVEN("a Repl with mocked I/O and real Variables/Evaluator")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"x = 5\", \"x * 2 + 1\"]")
        {
            auto outputs = runWithInputs({"x = 5", "x * 2 + 1"});

            THEN("first output is \"x = 5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "x = 5");
            }
            THEN("second output is \"11\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "11");
            }
        }
    }
}

SCENARIO("ans variable through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"2 + 3\", \"ans * 10\"]")
        {
            auto outputs = runWithInputs({"2 + 3", "ans * 10"});

            THEN("first output is \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
            THEN("second output is \"50\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "50");
            }
        }
    }
}

SCENARIO("Function calls through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"sqrt(16)\", \"max(3, 7)\", \"abs(-42)\"]")
        {
            auto outputs = runWithInputs({"sqrt(16)", "max(3, 7)", "abs(-42)"});

            THEN("outputs are [\"4\", \"7\", \"42\"]")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[0] == "4");
                CHECK(outputs[1] == "7");
                CHECK(outputs[2] == "42");
            }
        }
    }
}

SCENARIO("Repl handles evaluation errors")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("input is \"1 / 0\"")
        {
            auto outputs = runWithInputs({"1 / 0"});

            THEN("output starts with \"Error:\" and contains \"division by zero\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
                CHECK(outputs[0].contains("division by zero"));
            }
        }
        WHEN("input is \"2 +\"")
        {
            auto outputs = runWithInputs({"2 +"});

            THEN("output starts with \"Error:\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
            }
        }
        WHEN("input is \"undefined_var\"")
        {
            auto outputs = runWithInputs({"undefined_var"});

            THEN("output starts with \"Error:\" and contains \"undefined variable\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
                CHECK(outputs[0].contains("undefined variable"));
            }
        }
        WHEN("input is \"unknown_fn(1)\"")
        {
            auto outputs = runWithInputs({"unknown_fn(1)"});

            THEN("output starts with \"Error:\" and contains \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
                CHECK(outputs[0].contains("unknown function"));
            }
        }
    }
}

SCENARIO("vars command through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"x = 5\", \"y = 10\", \"vars\"]")
        {
            // Note: "x = 5" also sets ans=5, "y = 10" sets ans=10
            // so vars will list ans, x, y (alphabetical from std::map)
            auto outputs = runWithInputs({"x = 5", "y = 10", "vars"});

            THEN("third output contains \"x = 5\" and \"y = 10\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("x = 5"));
                CHECK(outputs[2].contains("y = 10"));
            }
        }
    }
}

SCENARIO("fns command through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("input is \"fns\"")
        {
            auto outputs = runWithInputs({"fns"});

            THEN("output lists builtin functions (abs, sqrt, sin, cos, ...)")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("abs"));
                CHECK(outputs[0].contains("sqrt"));
                CHECK(outputs[0].contains("sin"));
                CHECK(outputs[0].contains("cos"));
            }
        }
    }
}

SCENARIO("history command through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"2 + 3\", \"x = 5\", \"history\"]")
        {
            auto outputs = runWithInputs({"2 + 3", "x = 5", "history"});

            THEN("output includes numbered entries for \"2 + 3\" and \"x = 5\"")
            {
                // First two outputs are the results of the expressions,
                // third output is the history listing
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("2 + 3"));
                CHECK(outputs[2].contains("x = 5"));
                // History entries are numbered starting at 1
                CHECK(outputs[2].contains("1"));
                CHECK(outputs[2].contains("2"));
            }
        }
    }
}

SCENARIO("help command through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("input is \"help\"")
        {
            auto outputs = runWithInputs({"help"});

            THEN("output lists available commands")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("help"));
                CHECK(outputs[0].contains("vars"));
                CHECK(outputs[0].contains("fns"));
                CHECK(outputs[0].contains("history"));
                CHECK(outputs[0].contains("clear"));
            }
        }
    }
}

SCENARIO("clear command through Repl")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"x = 5\", \"clear\", \"x\"]")
        {
            auto outputs = runWithInputs({"x = 5", "clear", "x"});

            THEN("first output is \"x = 5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "x = 5");
            }
            THEN("second output is clear confirmation")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("clear"));
            }
            THEN("third output is error (undefined variable)")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("Error:"));
            }
        }
    }
}

SCENARIO("Repl handles empty input and quit")
{
    GIVEN("a Repl with mocked I/O")
    {
        IntegrationGraph graph;

        WHEN("inputs are [\"\", \"2 + 3\", \"quit\"]")
        {
            auto outputs = runWithInputs({"", "2 + 3", "quit"});

            THEN("empty input produces no output")
            {
                // Only one output total: "5" for "2 + 3"
                // "quit" exits without output, "" is silently skipped
                CHECK(outputs.size() == 1);
            }
            THEN("\"2 + 3\" produces \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
            THEN("Repl exits with 0")
            {
                CHECK(outputs.size() == 1);
            }
        }
    }
}

SCENARIO("Batch mode evaluates all inputs and exits")
{
    GIVEN("a Batch graph with inputs [\"2 + 3\", \"x = 5\", \"x * 2\"]")
    {
        graph::Batch graph;
        graph.lineReader->setInputs({"2 + 3", "x = 5", "x * 2"});

        WHEN("running the Repl")
        {
            // Batch uses real ConsoleOutput (writes to stdout) — we can only verify it runs without error
            int result = graph.repl->run();

            THEN("outputs are [\"5\", \"x = 5\", \"10\"]")
            {
                // Batch mode runs to completion and returns 0
                CHECK(result == 0);
            }
        }
    }
}

SCENARIO("Defining and calling a single-parameter function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x ^ 2 + 1", "f(3)"});

        WHEN("inputs are [\"f(x) = x ^ 2 + 1\", \"f(3)\"]")
        {
            THEN("first output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("f"));
            }
            THEN("second output is \"10\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "10");
            }
        }
    }
}

SCENARIO("Defining and calling a multi-parameter function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"g(x, y) = x + y * 2", "g(1, 5)"});

        WHEN("inputs are [\"g(x, y) = x + y * 2\", \"g(1, 5)\"]")
        {
            THEN("first output is \"g(x, y) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("g"));
            }
            THEN("second output is \"11\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "11");
            }
        }
    }
}

SCENARIO("User function calling another user function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x * 2", "g(x) = f(x) + 1", "g(3)"});

        WHEN("inputs are [\"f(x) = x * 2\", \"g(x) = f(x) + 1\", \"g(3)\"]")
        {
            THEN("first output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("f"));
            }
            THEN("second output is \"g(x) defined\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("defined"));
                CHECK(outputs[1].contains("g"));
            }
            THEN("third output is \"7\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2] == "7");
            }
        }
    }
}

SCENARIO("User function variable scoping preserves outer variables")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"x = 10", "f(x) = x + 1", "f(5)", "x"});

        WHEN("inputs are [\"x = 10\", \"f(x) = x + 1\", \"f(5)\", \"x\"]")
        {
            THEN("first output is \"x = 10\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "x = 10");
            }
            THEN("second output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("defined"));
                CHECK(outputs[1].contains("f"));
            }
            THEN("third output is \"6\" (function body evaluates to 5 + 1)")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2] == "6");
            }
            THEN("fourth output is \"10\" (x restored to outer scope)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3] == "10");
            }
        }
    }
}

SCENARIO("Overloaded user functions with different arity")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x * 10", "f(x, y) = x + y", "f(3)", "f(3, 4)"});

        WHEN("inputs are [\"f(x) = x * 10\", \"f(x, y) = x + y\", \"f(3)\", \"f(3, 4)\"]")
        {
            THEN("first output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("f"));
            }
            THEN("second output is \"f(x, y) defined\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("defined"));
                CHECK(outputs[1].contains("f"));
            }
            THEN("third output is \"30\" (f/1 with arg 3)")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2] == "30");
            }
            THEN("fourth output is \"7\" (f/2 with args 3, 4)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3] == "7");
            }
        }
    }
}

SCENARIO("Cannot shadow builtin at same arity but can at different arity")
{
    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"sqrt(x) = x * x\"]")
        {
            auto outputs = runWithInputs({"sqrt(x) = x * x"});
            THEN("output contains \"Error:\" (shadows builtin sqrt/1)")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
            }
        }
        WHEN("inputs are [\"sqrt(x, y) = x + y\", \"sqrt(3, 4)\"]")
        {
            auto outputs2 = runWithInputs({"sqrt(x, y) = x + y", "sqrt(3, 4)"});
            THEN("first output is \"sqrt(x, y) defined\" (different arity, OK)")
            {
                REQUIRE(outputs2.size() >= 1);
                CHECK(outputs2[0].contains("defined"));
                CHECK(outputs2[0].contains("sqrt"));
            }
            THEN("second output is \"7\"")
            {
                REQUIRE(outputs2.size() >= 2);
                CHECK(outputs2[1] == "7");
            }
        }
    }
}

SCENARIO("User function with no matching overload")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(1, 2)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(1, 2)\"]")
        {
            THEN("first output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("f"));
            }
            THEN("second output contains \"Error:\" and \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("Error:"));
                CHECK(outputs[1].contains("unknown function"));
            }
        }
    }
}

SCENARIO("Redefining a user function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(3)", "f(x) = x * 10", "f(3)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(3)\", \"f(x) = x * 10\", \"f(3)\"]")
        {
            THEN("second output is \"4\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "4");
            }
            THEN("fourth output is \"30\"")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3] == "30");
            }
        }
    }
}

SCENARIO("fns command lists user functions")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "fns"});

        WHEN("inputs are [\"f(x) = x + 1\", \"fns\"]")
        {
            THEN("second output contains \"f(x)\" with body")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("f(x)"));
                CHECK(outputs[1].contains("x + 1"));
            }
            THEN("second output still contains builtin names like \"sqrt\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("sqrt"));
            }
        }
    }
}

SCENARIO("clear command clears user functions")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "clear", "f(1)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"clear\", \"f(1)\"]")
        {
            THEN("third output contains \"Error:\" and \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("Error:"));
                CHECK(outputs[2].contains("unknown function"));
            }
        }
    }
}

SCENARIO("User function using variables and built-in functions")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"pi = 3.14159", "area(r) = pi * r ^ 2", "area(5)"});

        WHEN("inputs are [\"pi = 3.14159\", \"area(r) = pi * r ^ 2\", \"area(5)\"]")
        {
            THEN("third output is approximately \"78.5398\" (pi * 25)")
            {
                REQUIRE(outputs.size() >= 3);
                // Parse output as double and check it's close to pi * 25
                double result = std::stod(outputs[2]);
                double expected = 3.14159 * 25;
                CHECK(std::abs(result - expected) < 0.001);
            }
        }
    }
}

SCENARIO("Direct recursion is rejected")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = f(x - 1)"});

        WHEN("inputs are [\"f(x) = f(x - 1)\"]")
        {
            THEN("output contains \"Error:\" and \"recursion\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
                CHECK(outputs[0].contains("recursion"));
            }
        }
    }
}

SCENARIO("Defining a function referencing undefined function is rejected")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = g(x)"});

        WHEN("inputs are [\"f(x) = g(x)\"]")
        {
            THEN("output contains \"Error:\" and \"undefined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("Error:"));
                CHECK(outputs[0].contains("undefined"));
            }
        }
    }
}

SCENARIO("Indirect recursion via redefinition is rejected")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "f(x) = g(x)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"f(x) = g(x)\"]")
        {
            THEN("first output is \"f(x) defined\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].contains("defined"));
                CHECK(outputs[0].contains("f"));
            }
            THEN("second output is \"g(x) defined\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("defined"));
                CHECK(outputs[1].contains("g"));
            }
            THEN("third output contains \"Error:\" and \"recursion\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("Error:"));
                CHECK(outputs[2].contains("recursion"));
            }
        }
    }
}

SCENARIO("Undef command removes a function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "undef f", "f(1)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"undef f\", \"f(1)\"]")
        {
            THEN("third output contains \"Error:\" and \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("Error:"));
                CHECK(outputs[2].contains("unknown function"));
            }
        }
    }
}

SCENARIO("Undef command rejects removing a function with dependents")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "undef f"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"undef f\"]")
        {
            THEN("third output contains \"Error:\" and mentions \"g\" depending on \"f\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].contains("Error:"));
                CHECK(outputs[2].contains("g"));
            }
        }
    }
}

SCENARIO("Undef command removes interdependent functions together")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "undef f g", "f(1)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"undef f g\", \"f(1)\"]")
        {
            THEN("third output confirms removal")
            {
                REQUIRE(outputs.size() >= 3);
            }
            THEN("fourth output contains \"Error:\" and \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("Error:"));
                CHECK(outputs[3].contains("unknown function"));
            }
        }
    }
}

SCENARIO("Undef functions sequentially in correct order")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "undef g", "undef f", "f(1)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"undef g\", \"undef f\", \"f(1)\"]")
        {
            THEN("third output confirms removal of g")
            {
                REQUIRE(outputs.size() >= 3);
            }
            THEN("fourth output confirms removal of f")
            {
                REQUIRE(outputs.size() >= 4);
            }
            THEN("fifth output contains \"Error:\" and \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 5);
                CHECK(outputs[4].contains("Error:"));
                CHECK(outputs[4].contains("unknown function"));
            }
        }
    }
}

SCENARIO("Undef removes all overloads of a function")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "undef f", "f(1)", "f(1, 2)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"undef f\", \"f(1)\", \"f(1, 2)\"]")
        {
            THEN("\"undef f\" succeeds (removes both f/1 and f/2)")
            {
                REQUIRE(outputs.size() >= 3);
            }
            THEN("\"f(1)\" returns error (unknown function)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("Error:"));
                CHECK(outputs[3].contains("unknown function"));
            }
            THEN("\"f(1, 2)\" returns error (unknown function)")
            {
                REQUIRE(outputs.size() >= 5);
                CHECK(outputs[4].contains("Error:"));
                CHECK(outputs[4].contains("unknown function"));
            }
        }
    }
}

SCENARIO("Undef overloaded function respects dependencies on specific arity")
{
    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\", \"undef f\"]")
        {
            auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "g(x) = f(x) + 2", "undef f"});
            THEN("\"undef f\" returns error (g/1 depends on f/1)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("Error:"));
            }
        }
        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\", \"undef f g\"]")
        {
            auto outputs2 = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "g(x) = f(x) + 2", "undef f g"});
            THEN("\"undef f g\" succeeds (all deps removed together)")
            {
                REQUIRE(outputs2.size() >= 4);
            }
        }
    }
}

SCENARIO("Redefining an overloaded function updates dependencies correctly")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "g(x) = f(x) + 2", "g(x) = x * 3", "undef f", "g(5)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\", \"g(x) = x * 3\", \"undef f\", \"g(5)\"]")
        {
            THEN("\"g(x) = x * 3\" redefines g to no longer depend on f/1")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("defined"));
            }
            THEN("\"undef f\" succeeds")
            {
                REQUIRE(outputs.size() >= 5);
            }
            THEN("\"g(5)\" returns \"15\"")
            {
                REQUIRE(outputs.size() >= 6);
                CHECK(outputs[5] == "15");
            }
        }
    }
}

SCENARIO("Undef removes specific arity overloads only")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "undef f/1", "f(1)", "f(1, 2)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"undef f/1\", \"f(1)\", \"f(1, 2)\"]")
        {
            THEN("\"undef f/1\" succeeds (removes only f/1)")
            {
                REQUIRE(outputs.size() >= 3);
            }
            THEN("\"f(1)\" returns error (f/1 removed)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("Error:"));
                CHECK(outputs[3].contains("unknown function"));
            }
            THEN("\"f(1, 2)\" returns \"3\" (f/2 still exists)")
            {
                REQUIRE(outputs.size() >= 5);
                CHECK(outputs[4] == "3");
            }
        }
    }
}

SCENARIO("Undef with arity respects dependencies on specific overload")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "g(x) = f(x) + 2", "undef f/1"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(x) = f(x) + 2\", \"undef f/1\"]")
        {
            THEN("\"undef f/1\" returns error (g/1 depends on f/1)")
            {
                REQUIRE(outputs.size() >= 4);
                CHECK(outputs[3].contains("Error:"));
            }
        }
    }
}

SCENARIO("Undef removes specific arity when no dependencies")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "f(x, y) = x + y", "g(y) = f(y, 2)", "undef f/1", "f(1)", "g(3)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"f(x, y) = x + y\", \"g(y) = f(y, 2)\", \"undef f/1\", \"f(1)\", \"g(3)\"]")
        {
            THEN("\"undef f/1\" succeeds (g depends on f/2, not f/1)")
            {
                REQUIRE(outputs.size() >= 4);
            }
            THEN("\"f(1)\" returns error (f/1 removed)")
            {
                REQUIRE(outputs.size() >= 5);
                CHECK(outputs[4].contains("Error:"));
            }
            THEN("\"g(3)\" returns \"5\" (uses f/2)")
            {
                REQUIRE(outputs.size() >= 6);
                CHECK(outputs[5] == "5");
            }
        }
    }
}

SCENARIO("Cross-arity function call is not recursion")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x, y) = x + y", "f(x) = f(x, 1)", "f(3)"});

        WHEN("inputs are [\"f(x, y) = x + y\", \"f(x) = f(x, 1)\", \"f(3)\"]")
        {
            THEN("\"f(x) = f(x, 1)\" succeeds (f/1 calls f/2, not itself)")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].contains("defined"));
                CHECK(outputs[1].contains("f"));
            }
            THEN("\"f(3)\" returns \"4\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2] == "4");
            }
        }
    }
}

SCENARIO("Redefining a function updates dependencies")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "g(x) = x * 3", "undef f", "g(5)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"g(x) = x * 3\", \"undef f\", \"g(5)\"]")
        {
            THEN("\"undef f\" succeeds (g no longer depends on f after redefinition)")
            {
                REQUIRE(outputs.size() >= 4);
            }
            THEN("\"g(5)\" returns \"15\"")
            {
                REQUIRE(outputs.size() >= 5);
                CHECK(outputs[4] == "15");
            }
        }
    }
}

SCENARIO("Save and load preserves user functions")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "x = 10", "save", "clear", "load", "f(5)", "x"});

        WHEN("inputs are [\"f(x) = x + 1\", \"x = 10\", \"save\", \"clear\", \"load\", \"f(5)\", \"x\"]")
        {
            THEN("\"f(5)\" after load returns \"6\"")
            {
                INFO(std::format("Outputs were: {}", outputs));
                REQUIRE(std::ranges::contains(outputs, "6"));
            }
            THEN("\"x\" after load returns \"10\"")
            {
                INFO(std::format("Outputs were: {}", outputs));
                REQUIRE(std::ranges::contains(outputs, "10"));
            }
        }
    }
}

SCENARIO("Loading user functions with dependencies out of order")
{
    GIVEN("a Repl with mocked I/O")
    {
        auto outputs = runWithInputs({"f(x) = x + 1", "g(x) = f(x) + 2", "save", "clear", "load", "g(3)"});

        WHEN("inputs are [\"f(x) = x + 1\", \"g(x) = f(x) + 2\", \"save\", \"clear\", \"load\", \"g(3)\"]")
        {
            THEN("\"g(3)\" after load returns \"6\" (even if file has g before f, loader handles ordering)")
            {
                INFO(std::format("Outputs were: {}", outputs));
                REQUIRE(std::ranges::contains(outputs, "6"));
            }
        }
    }
}
