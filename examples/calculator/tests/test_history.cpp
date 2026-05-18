import examples.calculator.node.history_store;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#include "arc/doctest.h"

using namespace examples::calculator;

SCENARIO("Adding and retrieving history entries")
{
    GIVEN("an empty HistoryStore node")
    {
        arc::test::Graph<node::HistoryStore> graph;
        auto history = graph.asTrait(trait::history);
        WHEN("addEntry(\"2 + 3\")")
        {
            history.addEntry("2 + 3");

            THEN("entries() returns [\"2 + 3\"]")
            {
                auto result = history.entries();
                REQUIRE(result.size() == 1);
                CHECK(result[0] == "2 + 3");
            }
            AND_WHEN("addEntry(\"x = 5\")")
            {
                history.addEntry("x = 5");

                THEN("entries() returns [\"2 + 3\", \"x = 5\"] in insertion order")
                {
                    auto result = history.entries();
                    REQUIRE(result.size() == 2);
                    CHECK(result[0] == "2 + 3");
                    CHECK(result[1] == "x = 5");
                }
            }
        }
    }
}

SCENARIO("Empty history")
{
    GIVEN("an empty HistoryStore node")
    {
        arc::test::Graph<node::HistoryStore> graph;
        auto history = graph.asTrait(trait::history);
        WHEN("calling entries()")
        {
            auto result = history.entries();

            THEN("returns empty vector")
            {
                CHECK(result.empty());
            }
        }
    }
}
