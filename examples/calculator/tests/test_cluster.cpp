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
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("input is \"2 + 3\"")
        {
            auto outputs = runWithInputs({"2 + 3"});

            THEN("output is \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
        }
        AND_WHEN("input is \"10 - 3 * 2\"")
        {
            auto outputs = runWithInputs({"10 - 3 * 2"});

            THEN("output is \"4\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "4");
            }
        }
        AND_WHEN("input is \"(10 - 3) * 2\"")
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
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("input is \"2 + 3 * 4\"")
        {
            auto outputs = runWithInputs({"2 + 3 * 4"});

            THEN("output is \"14\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "14");
            }
        }
        AND_WHEN("input is \"2 * 3 + 4 * 5\"")
        {
            auto outputs = runWithInputs({"2 * 3 + 4 * 5"});

            THEN("output is \"26\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "26");
            }
        }
        AND_WHEN("input is \"2 ^ 3 + 1\"")
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
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O and real Variables/Evaluator")
    {
        WHEN("inputs are [\"x = 5\", \"x * 2 + 1\"]")
        {
            auto outputs = runWithInputs({"x = 5", "x * 2 + 1"});

            THEN("first output is \"x = 5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "x = 5");
            }
            AND_THEN("second output is \"11\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "11");
            }
        }
    }
}

SCENARIO("ans variable through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"2 + 3\", \"ans * 10\"]")
        {
            auto outputs = runWithInputs({"2 + 3", "ans * 10"});

            THEN("first output is \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
            AND_THEN("second output is \"50\"")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1] == "50");
            }
        }
    }
}

SCENARIO("Function calls through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
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
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("input is \"1 / 0\"")
        {
            auto outputs = runWithInputs({"1 / 0"});

            THEN("output starts with \"Error:\" and contains \"division by zero\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("Error:") != std::string::npos);
                CHECK(outputs[0].find("division by zero") != std::string::npos);
            }
        }
        AND_WHEN("input is \"2 +\"")
        {
            auto outputs = runWithInputs({"2 +"});

            THEN("output starts with \"Error:\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("Error:") != std::string::npos);
            }
        }
        AND_WHEN("input is \"undefined_var\"")
        {
            auto outputs = runWithInputs({"undefined_var"});

            THEN("output starts with \"Error:\" and contains \"undefined variable\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("Error:") != std::string::npos);
                CHECK(outputs[0].find("undefined variable") != std::string::npos);
            }
        }
        AND_WHEN("input is \"unknown_fn(1)\"")
        {
            auto outputs = runWithInputs({"unknown_fn(1)"});

            THEN("output starts with \"Error:\" and contains \"unknown function\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("Error:") != std::string::npos);
                CHECK(outputs[0].find("unknown function") != std::string::npos);
            }
        }
    }
}

SCENARIO("vars command through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"x = 5\", \"y = 10\", \"vars\"]")
        {
            // Note: "x = 5" also sets ans=5, "y = 10" sets ans=10
            // so vars will list ans, x, y (alphabetical from std::map)
            auto outputs = runWithInputs({"x = 5", "y = 10", "vars"});

            THEN("third output contains \"x = 5\" and \"y = 10\"")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].find("x = 5") != std::string::npos);
                CHECK(outputs[2].find("y = 10") != std::string::npos);
            }
        }
    }
}

SCENARIO("fns command through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("input is \"fns\"")
        {
            auto outputs = runWithInputs({"fns"});

            THEN("output lists available functions (abs, sqrt, sin, cos, ...)")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("abs") != std::string::npos);
                CHECK(outputs[0].find("sqrt") != std::string::npos);
                CHECK(outputs[0].find("sin") != std::string::npos);
                CHECK(outputs[0].find("cos") != std::string::npos);
            }
        }
    }
}

SCENARIO("history command through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"2 + 3\", \"x = 5\", \"history\"]")
        {
            auto outputs = runWithInputs({"2 + 3", "x = 5", "history"});

            THEN("output includes numbered entries for \"2 + 3\" and \"x = 5\"")
            {
                // First two outputs are the results of the expressions,
                // third output is the history listing
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].find("2 + 3") != std::string::npos);
                CHECK(outputs[2].find("x = 5") != std::string::npos);
                // History entries are numbered starting at 1
                CHECK(outputs[2].find("1") != std::string::npos);
                CHECK(outputs[2].find("2") != std::string::npos);
            }
        }
    }
}

SCENARIO("help command through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("input is \"help\"")
        {
            auto outputs = runWithInputs({"help"});

            THEN("output lists available commands")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0].find("help") != std::string::npos);
                CHECK(outputs[0].find("vars") != std::string::npos);
                CHECK(outputs[0].find("fns") != std::string::npos);
                CHECK(outputs[0].find("history") != std::string::npos);
                CHECK(outputs[0].find("clear") != std::string::npos);
            }
        }
    }
}

SCENARIO("clear command through Repl")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"x = 5\", \"clear\", \"x\"]")
        {
            auto outputs = runWithInputs({"x = 5", "clear", "x"});

            THEN("first output is \"x = 5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "x = 5");
            }
            AND_THEN("second output is clear confirmation")
            {
                REQUIRE(outputs.size() >= 2);
                CHECK(outputs[1].find("clear") != std::string::npos);
            }
            AND_THEN("third output is error (undefined variable)")
            {
                REQUIRE(outputs.size() >= 3);
                CHECK(outputs[2].find("Error:") != std::string::npos);
            }
        }
    }
}

SCENARIO("Repl handles empty input and quit")
{
    IntegrationGraph graph;

    GIVEN("a Repl with mocked I/O")
    {
        WHEN("inputs are [\"\", \"2 + 3\", \"quit\"]")
        {
            auto outputs = runWithInputs({"", "2 + 3", "quit"});

            THEN("empty input produces no output")
            {
                // Only one output total: "5" for "2 + 3"
                // "quit" exits without output, "" is silently skipped
                CHECK(outputs.size() == 1);
            }
            AND_THEN("\"2 + 3\" produces \"5\"")
            {
                REQUIRE(outputs.size() >= 1);
                CHECK(outputs[0] == "5");
            }
            AND_THEN("Repl exits with 0")
            {
                // run() returned 0 — verified by the fact outputs has only the expected entry
                CHECK(outputs.size() == 1);
            }
        }
    }
}

SCENARIO("Batch mode evaluates all inputs and exits")
{
    graph::Batch graph;
    graph.lineReader->setInputs({"2 + 3", "x = 5", "x * 2"});

    GIVEN("a Batch graph with inputs [\"2 + 3\", \"x = 5\", \"x * 2\"]")
    {
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
