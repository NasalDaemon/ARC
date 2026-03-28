#include <doctest/doctest.h>

import examples.calculator.batch_line_reader;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("BatchLineReader returns configured inputs then EOF")
{
    arc::test::Graph<node::BatchLineReader> graph;
    auto reader = graph.asTrait(trait::lineReader);

    GIVEN("a BatchLineReader with inputs [\"2 + 3\", \"x = 5\"]")
    {
        graph.node->setInputs({"2 + 3", "x = 5"});

        WHEN("reading all inputs in sequence")
        {
            auto result1 = reader.readLine(">");
            auto result2 = reader.readLine(">");
            auto result3 = reader.readLine(">");

            THEN("first call returns \"2 + 3\"")
            {
                REQUIRE(result1.has_value());
                CHECK(*result1 == "2 + 3");
            }
            AND_THEN("second call returns \"x = 5\"")
            {
                REQUIRE(result2.has_value());
                CHECK(*result2 == "x = 5");
            }
            AND_THEN("third call returns nullopt (EOF)")
            {
                CHECK_FALSE(result3.has_value());
            }
        }
    }
}

SCENARIO("BatchLineReader with no inputs")
{
    arc::test::Graph<node::BatchLineReader> graph;
    auto reader = graph.asTrait(trait::lineReader);

    GIVEN("a BatchLineReader with empty inputs")
    {
        graph.node->setInputs({});

        WHEN("calling readLine(\">\")")
        {
            auto result = reader.readLine(">");

            THEN("returns nullopt immediately")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}
