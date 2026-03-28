#include <doctest/doctest.h>

import examples.calculator.command_handler;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

using namespace examples::calculator;

SCENARIO("help command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        WHEN("executing \"help\"")
        {
            auto result = commands.execute("help");

            THEN("returns a string listing available commands")
            {
                CHECK(result.has_value());
                auto output = *result;
                AND_THEN("output contains \"vars\", \"fns\", \"history\", \"save\", \"load\", \"clear\", \"help\"")
                {
                    CHECK(output.find("vars") != std::string::npos);
                    CHECK(output.find("fns") != std::string::npos);
                    CHECK(output.find("history") != std::string::npos);
                    CHECK(output.find("save") != std::string::npos);
                    CHECK(output.find("load") != std::string::npos);
                    CHECK(output.find("clear") != std::string::npos);
                    CHECK(output.find("help") != std::string::npos);
                }
            }
        }
    }
}

SCENARIO("vars command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock Variables returning [x=5, y=10]")
    {
        graph.mocks->define(
            [](trait::Variables::list) -> std::vector<std::pair<std::string, double>>
            {
                return {{"x", 5.0}, {"y", 10.0}};
            }
        );
        graph.mocks->define(
            [](trait::Formatter::formatVariables, std::span<std::pair<std::string, double> const> vars) -> std::string
            {
                std::string out;
                for (auto const& [name, value] : vars)
                    out += name + "=" + std::to_string(value) + "\n";
                return out;
            }
        );

        WHEN("executing \"vars\"")
        {
            auto result = commands.execute("vars");

            THEN("returns formatted variable listing via mock Formatter")
            {
                CHECK(result.has_value());
                CHECK(result->find("x") != std::string::npos);
                CHECK(result->find("y") != std::string::npos);
            }
        }
    }
}

SCENARIO("fns command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock Functions returning [\"abs\", \"sqrt\"]")
    {
        graph.mocks->define(
            [](trait::Functions::list) -> std::vector<std::string>
            {
                return {"abs", "sqrt"};
            }
        );
        graph.mocks->define(
            [](trait::Formatter::formatFunctions, std::span<std::string const> names) -> std::string
            {
                std::string out;
                for (auto const& name : names)
                    out += name + "\n";
                return out;
            }
        );

        WHEN("executing \"fns\"")
        {
            auto result = commands.execute("fns");

            THEN("returns formatted function listing via mock Formatter")
            {
                CHECK(result.has_value());
                CHECK(result->find("abs") != std::string::npos);
                CHECK(result->find("sqrt") != std::string::npos);
            }
        }
    }
}

SCENARIO("history command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock History returning [\"2 + 3\", \"x = 5\"]")
    {
        graph.mocks->define(
            [](trait::History::entries) -> std::vector<std::string>
            {
                return {"2 + 3", "x = 5"};
            }
        );

        WHEN("executing \"history\"")
        {
            auto result = commands.execute("history");

            THEN("returns numbered list: \"  1  2 + 3\\n  2  x = 5\"")
            {
                CHECK(result.has_value());
                CHECK(result->find("2 + 3") != std::string::npos);
                CHECK(result->find("x = 5") != std::string::npos);
                CHECK(result->find("1") != std::string::npos);
                CHECK(result->find("2") != std::string::npos);
            }
        }
    }
}

SCENARIO("clear command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    graph.mocks->enableCallCounting();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        WHEN("executing \"clear\"")
        {
            auto result = commands.execute("clear");

            THEN("mock Variables::clear() was called")
            {
                CHECK(graph.mocks->methodCallCount<trait::Variables::clear>() == 1);
            }
            AND_THEN("returns confirmation message")
            {
                CHECK(result.has_value());
            }
        }
    }
}

SCENARIO("save command with path")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](trait::Persistence::save, std::string_view path) -> std::expected<void, std::string>
            {
                capturedPath = std::string(path);
                return {};
            }
        );

        WHEN("executing \"save mystate.txt\"")
        {
            auto result = commands.execute("save mystate.txt");

            THEN("returns expected with success message")
            {
                CHECK(result.has_value());
            }
            AND_THEN("mock Persistence::save() was called with \"mystate.txt\"")
            {
                CHECK(capturedPath == "mystate.txt");
            }
        }
    }
}

SCENARIO("save command without path")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](trait::Persistence::save, std::string_view path) -> std::expected<void, std::string>
            {
                capturedPath = std::string(path);
                return {};
            }
        );

        WHEN("executing \"save\"")
        {
            auto result = commands.execute("save");

            THEN("returns expected with success message")
            {
                CHECK(result.has_value());
            }
            AND_THEN("mock Persistence::save() was called with default path \"calculator.state\"")
            {
                CHECK(capturedPath == "calculator.state");
            }
        }
    }
}

SCENARIO("save command when persistence fails")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock Persistence::save returning error")
    {
        graph.mocks->define(
            [](trait::Persistence::save, std::string_view) -> std::expected<void, std::string>
            {
                return std::unexpected(std::string("Disk full"));
            }
        );

        WHEN("executing \"save\"")
        {
            auto result = commands.execute("save");

            THEN("returns unexpected with error message")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("load command")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock Persistence::load succeeding")
    {
        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](trait::Persistence::load, std::string_view path) -> std::expected<void, std::string>
            {
                capturedPath = std::string(path);
                return {};
            }
        );

        WHEN("executing \"load mystate.txt\"")
        {
            auto result = commands.execute("load mystate.txt");

            THEN("returns expected with success message")
            {
                CHECK(result.has_value());
            }
            AND_THEN("mock Persistence::load() was called with \"mystate.txt\"")
            {
                CHECK(capturedPath == "mystate.txt");
            }
        }
    }
}

SCENARIO("load command when persistence fails")
{
    arc::test::Graph<node::CommandHandler> graph;
    graph.mocks->setReturnDefault();
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler with mock Persistence::load returning error")
    {
        graph.mocks->define(
            [](trait::Persistence::load, std::string_view) -> std::expected<void, std::string>
            {
                return std::unexpected(std::string("File not found"));
            }
        );

        WHEN("executing \"load\"")
        {
            auto result = commands.execute("load");

            THEN("returns unexpected with error message")
            {
                CHECK_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("isCommand recognises known commands")
{
    arc::test::Graph<node::CommandHandler> graph;
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        WHEN("checking \"help\"")
        {
            auto result = commands.isCommand("help");

            THEN("isCommand returns true")
            {
                CHECK(result);
            }
        }
        AND_WHEN("checking \"vars\"")
        {
            auto result = commands.isCommand("vars");

            THEN("isCommand returns true")
            {
                CHECK(result);
            }
        }
        AND_WHEN("checking \"save myfile.txt\"")
        {
            auto result = commands.isCommand("save myfile.txt");

            THEN("isCommand returns true (matches first word)")
            {
                CHECK(result);
            }
        }
    }
}

SCENARIO("isCommand rejects non-commands")
{
    arc::test::Graph<node::CommandHandler> graph;
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        WHEN("checking \"2 + 3\"")
        {
            auto result = commands.isCommand("2 + 3");

            THEN("isCommand returns false")
            {
                CHECK_FALSE(result);
            }
        }
        AND_WHEN("checking \"x = 5\"")
        {
            auto result = commands.isCommand("x = 5");

            THEN("isCommand returns false")
            {
                CHECK_FALSE(result);
            }
        }
    }
}

SCENARIO("Commands contract: execute requires isCommand")
{
    arc::test::Graph<node::CommandHandler> graph;
    auto commands = graph.asTrait(trait::commands);

    GIVEN("a CommandHandler node")
    {
        WHEN("calling execute(\"2 + 3\") which is not a command")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(commands.execute("2 + 3"), arc::ContractViolation);
            }
        }
    }
}
