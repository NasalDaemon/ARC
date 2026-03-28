#include <doctest/doctest.h>

import examples.calculator.file_persistence;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("Saving variables to file")
{
    arc::test::Graph<node::FilePersistence> graph;
    auto persistence = graph.asTrait(trait::persistence);

    GIVEN("a FilePersistence node with mock Variables returning [x=5, y=10]")
    {
        graph.mocks->methodReturns<trait::Variables::list>(
            std::vector<std::pair<std::string, double>>{
                {"x", 5.0}, {"y", 10.0}
            });

        WHEN("saving to a temp file")
        {
            auto result = persistence.save("/tmp/test_calc_save.state");

            THEN("returns success")
            {
                CHECK(result.has_value());
            }
            AND_THEN("file contains \"x=5\" and \"y=10\" lines")
            {
                std::ifstream file("/tmp/test_calc_save.state");
                CHECK(file.is_open());
                std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                CHECK(contents.find("x=") != std::string::npos);
                CHECK(contents.find("y=") != std::string::npos);
            }
        }
    }
}

SCENARIO("Loading variables from file")
{
    arc::test::Graph<node::FilePersistence> graph;
    graph.mocks->setReturnDefault();
    graph.mocks->enableCallCounting();
    auto persistence = graph.asTrait(trait::persistence);

    GIVEN("a FilePersistence node and a temp file with \"x=5\" and \"y=10\"")
    {
        // Create the temp file for loading
        {
            std::ofstream f("/tmp/test_calc_load.state");
            f << "x=5\ny=10\n";
        }

        std::vector<std::pair<std::string, double>> setCalls;
        graph.mocks->define(
            [&setCalls](trait::Variables::set, std::string name, double value)
            {
                setCalls.emplace_back(std::move(name), value);
            }
        );

        WHEN("loading from the file")
        {
            auto result = persistence.load("/tmp/test_calc_load.state");

            THEN("returns success")
            {
                CHECK(result.has_value());
            }
            AND_THEN("mock Variables::clear() was called")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::clear>() == 1);
            }
            AND_THEN("mock Variables::set() was called with (\"x\", 5) and (\"y\", 10)")
            {
                CHECK(setCalls.size() == 2);
                auto xIt = std::ranges::find_if(setCalls, [](auto const& p){ return p.first == "x"; });
                auto yIt = std::ranges::find_if(setCalls, [](auto const& p){ return p.first == "y"; });
                CHECK(xIt != setCalls.end());
                CHECK(yIt != setCalls.end());
                if (xIt != setCalls.end())
                    CHECK(xIt->second == doctest::Approx(5.0));
                if (yIt != setCalls.end())
                    CHECK(yIt->second == doctest::Approx(10.0));
            }
        }
    }
}

SCENARIO("Loading from nonexistent file")
{
    arc::test::Graph<node::FilePersistence> graph;
    auto persistence = graph.asTrait(trait::persistence);

    GIVEN("a FilePersistence node")
    {
        WHEN("loading from \"/tmp/nonexistent_calc_test\"")
        {
            auto result = persistence.load("/tmp/nonexistent_calc_test");

            THEN("returns error string")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Saving to invalid path")
{
    arc::test::Graph<node::FilePersistence> graph;
    auto persistence = graph.asTrait(trait::persistence);

    GIVEN("a FilePersistence node with mock Variables returning empty list")
    {
        WHEN("saving to \"/nonexistent_dir/file\"")
        {
            auto result = persistence.save("/nonexistent_dir/file");

            THEN("returns error string")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}
