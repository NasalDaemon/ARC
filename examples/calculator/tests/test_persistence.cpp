#include <doctest/doctest.h>

import examples.calculator.node.file_persistence;
import examples.calculator.tests.graphs;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;
using namespace examples::calculator::tests;

namespace {

struct TempDir
{
    std::filesystem::path path;

    explicit TempDir(std::string const& label)
        : path{std::filesystem::temp_directory_path() / label}
    {
        std::filesystem::create_directories(path);
    }

    ~TempDir()
    {
        std::filesystem::remove_all(path);
    }

    TempDir(TempDir const&) = delete;
    TempDir& operator=(TempDir const&) = delete;

    auto file(std::string const& name) const -> std::filesystem::path
    {
        return path / name;
    }
};

}

SCENARIO("Saving variables to file")
{
    GIVEN("a FilePersistence node with mock Variables returning [x=5, y=10]")
    {
        TempDir tmp{"calc_test_save"};
        arc::test::Graph<node::FilePersistence> graph;
        graph.mocks->setReturnDefault();
        auto persistence = graph.asTrait(trait::persistence);

        graph.mocks->methodReturns<Variables::list>(
            std::vector<std::pair<std::string, double>>{
                {"x", 5.0}, {"y", 10.0}
            });

        graph.mocks->methodReturns<UserFunctions::list>(
            std::vector<std::pair<std::string, UserFunction const*>>{});

        WHEN("saving to a temp file")
        {
            auto filePath = tmp.file("save.state");
            auto result = persistence.save(filePath.string());

            THEN("returns success")
            {
                CHECK(result.has_value());
            }
            THEN("file contains \"x=\" and \"y=\" lines")
            {
                std::ifstream file(filePath);
                CHECK(file.is_open());
                std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                CHECK(contents.contains("x="));
                CHECK(contents.contains("y="));
            }
        }
    }
}

SCENARIO("Loading variables from file")
{
    GIVEN("a FilePersistence node and a temp file with \"x=5\" and \"y=10\"")
    {
        TempDir tmp{"calc_test_load"};
        arc::test::Graph<node::FilePersistence> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto persistence = graph.asTrait(trait::persistence);

        auto filePath = tmp.file("load.state");
        {
            std::ofstream f(filePath);
            f << "x=5\ny=10\n";
        }

        std::vector<std::pair<std::string, double>> setCalls;
        graph.mocks->define(
            [&setCalls](Variables::set, std::string name, double value)
            {
                setCalls.emplace_back(std::move(name), value);
            }
        );

        WHEN("loading from the file")
        {
            auto result = persistence.load(filePath.string());

            THEN("returns success")
            {
                CHECK(result.has_value());
            }
            THEN("mock Variables::clear() was called")
            {
                CHECK(graph.mocks->methodCallCount<Variables::clear>() == 1);
            }
            THEN("mock Variables::set() was called with (\"x\", 5) and (\"y\", 10)")
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
    GIVEN("a FilePersistence node")
    {
        arc::test::Graph<node::FilePersistence> graph;
        auto persistence = graph.asTrait(trait::persistence);
        WHEN("loading from a nonexistent path")
        {
            auto nonexistent = std::filesystem::temp_directory_path() / "nonexistent_calc_test";
            auto result = persistence.load(nonexistent.string());

            THEN("returns error string")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("Saving to invalid path")
{
    GIVEN("a FilePersistence node with mock Variables returning empty list")
    {
        arc::test::Graph<node::FilePersistence> graph;
        auto persistence = graph.asTrait(trait::persistence);
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

SCENARIO("Saving user functions")
{
    TempDir tmp{"calc_test_funcs"};
    GIVEN("a FilePersistence node with mocked Variables and Functions")
    {
        arc::test::Graph<node::FilePersistence> graph;
        graph.mocks->setReturnDefault();
        auto persistence = graph.asTrait(trait::persistence);

        graph.mocks->methodReturns<Variables::list>(
            std::vector<std::pair<std::string, double>>{
                {"x", 5.0}, {"y", 10.0}
            });

        graph.mocks->methodReturns<UserFunctions::list>(
            std::vector<std::pair<std::string, UserFunction const*>>{});

        WHEN("saving to a file path")
        {
            auto result = persistence.save(tmp.file("funcs.state").string());

            THEN("success is returned")
            {
                CHECK(result.has_value());
            }
        }
    }
}

SCENARIO("Loading interdependent functions in any file order")
{
    GIVEN("a state file with g(x)=f(x)+2 listed before its dependency f(x)=x+1")
    {
        TempDir tmp{"calc_test_reverse_dep"};
        auto filePath = tmp.file("reverse.state");
        {
            std::ofstream file(filePath);
            file << "fn:g(x)=f(x) + 2\n";
            file << "fn:f(x)=x + 1\n";
        }

        WHEN("loading the file and evaluating g(3)")
        {
            IntegrationGraph graph;
            graph.lineReader->setInputs({"load " + filePath.string(), "g(3)"});
            graph.repl->run();
            auto const& outputs = graph.output->lines();

            THEN("g(3) returns 6, showing both functions loaded correctly despite reverse order")
            {
                INFO(std::format("Outputs were: {}", outputs));
                REQUIRE(std::ranges::contains(outputs, "6"));
            }
        }
    }

    GIVEN("a state file with a three-level chain h→g→f listed in fully reverse dependency order")
    {
        TempDir tmp{"calc_test_triple_dep"};
        auto filePath = tmp.file("triple.state");
        {
            std::ofstream file(filePath);
            file << "fn:h(x)=g(x) + 3\n";
            file << "fn:g(x)=f(x) + 2\n";
            file << "fn:f(x)=x + 1\n";
        }

        WHEN("loading the file and evaluating h(1)")
        {
            IntegrationGraph graph;
            graph.lineReader->setInputs({"load " + filePath.string(), "h(1)"});
            graph.repl->run();
            auto const& outputs = graph.output->lines();

            THEN("h(1) returns 7 (= (1+1) + 2 + 3), showing all three functions loaded correctly")
            {
                INFO(std::format("Outputs were: {}", outputs));
                REQUIRE(std::ranges::contains(outputs, "7"));
            }
        }
    }
}
