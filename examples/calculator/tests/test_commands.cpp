import examples.calculator.node.command_handler;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#include "doctest.h"

using namespace examples::calculator;

SCENARIO("help command")
{
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);
        WHEN("executing \"help\"")
        {
            auto result = commands.execute("help");

            THEN("returns a string listing available commands")
            {
                CHECK(result.has_value());
            }
            THEN("output contains \"vars\", \"fns\", \"history\", \"save\", \"load\", \"clear\", \"help\", \"undef\"")
            {
                CHECK(result->find("vars") != std::string::npos);
                CHECK(result->find("fns") != std::string::npos);
                CHECK(result->find("history") != std::string::npos);
                CHECK(result->find("save") != std::string::npos);
                CHECK(result->find("load") != std::string::npos);
                CHECK(result->find("clear") != std::string::npos);
                CHECK(result->find("help") != std::string::npos);
                CHECK(result->find("undef") != std::string::npos);
            }
        }
    }
}

SCENARIO("vars command")
{
    GIVEN("a CommandHandler with mock Variables returning [x=5, y=10]")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        graph.mocks->methodReturns<Variables::list>(
            std::vector<std::pair<std::string, double>>{
                {"x", 5.0}, {"y", 10.0}
            });
        graph.mocks->define(
            [](Formatter::formatVariables, std::span<std::pair<std::string, double> const> vars) -> std::string
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
    GIVEN("a CommandHandler with mock Functions returning [\"abs\", \"sqrt\"] and no user functions")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        graph.mocks->methodReturns<BuiltinFunctions::list>(
            std::vector<std::string>{"abs", "sqrt"});
        graph.mocks->methodReturns<UserFunctions::list>(
            std::vector<std::pair<std::string, UserFunction const*>>{});
        graph.mocks->define(
            [](Formatter::formatFunctions, std::span<std::string const> builtins, std::span<std::pair<std::string, UserFunction const*> const>) -> std::string
            {
                std::string out;
                for (auto const& name : builtins)
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
    GIVEN("a CommandHandler with mock History returning [\"2 + 3\", \"x = 5\"]")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        graph.mocks->methodReturns<History::entries>(
            std::vector<std::string>{"2 + 3", "x = 5"});

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
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto commands = graph.asTrait(trait::commands);
        WHEN("executing \"clear\"")
        {
            auto result = commands.execute("clear");

            THEN("mock Variables::clear() was called")
            {
                CHECK(graph.mocks->methodCallCount<Variables::clear>() == 1);
            }
            THEN("returns confirmation message")
            {
                CHECK(result.has_value());
            }
        }
    }
}

SCENARIO("save command with path")
{
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](Persistence::save, std::string_view path) -> std::expected<void, std::string>
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
            THEN("mock Persistence::save() was called with \"mystate.txt\"")
            {
                CHECK(capturedPath == "mystate.txt");
            }
        }
    }
}

SCENARIO("save command without path")
{
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](Persistence::save, std::string_view path) -> std::expected<void, std::string>
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
            THEN("mock Persistence::save() was called with default path \"calculator.state\"")
            {
                CHECK(capturedPath == "calculator.state");
            }
        }
    }
}

SCENARIO("save command when persistence fails")
{
    GIVEN("a CommandHandler with mock Persistence::save returning error")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        graph.mocks->methodReturns<Persistence::save>(
            std::expected<void, std::string>{std::unexpect, "Disk full"});

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
    GIVEN("a CommandHandler with mock Persistence::load succeeding")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        std::string capturedPath;
        graph.mocks->define(
            [&capturedPath](Persistence::load, std::string_view path) -> std::expected<void, std::string>
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
            THEN("mock Persistence::load() was called with \"mystate.txt\"")
            {
                CHECK(capturedPath == "mystate.txt");
            }
        }
    }
}

SCENARIO("load command when persistence fails")
{
    GIVEN("a CommandHandler with mock Persistence::load returning error")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);

        graph.mocks->methodReturns<Persistence::load>(
            std::expected<void, std::string>{std::unexpect, "File not found"});

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
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->methodReturns<BuiltinFunctions::list>(
            std::vector<std::string>{"abs", "sqrt", "sin"});
        auto commands = graph.asTrait(trait::commands);
        WHEN("checking \"help\"")
        {
            auto result = commands.isCommand("help");

            THEN("isCommand returns true")
            {
                CHECK(result);
            }
        }
        WHEN("checking \"vars\"")
        {
            auto result = commands.isCommand("vars");

            THEN("isCommand returns true")
            {
                CHECK(result);
            }
        }
        WHEN("checking \"save myfile.txt\"")
        {
            auto result = commands.isCommand("save myfile.txt");

            THEN("isCommand returns true (matches first word)")
            {
                CHECK(result);
            }
        }
        WHEN("checking \"sqrt\"")
        {
            auto result = commands.isCommand("sqrt");

            THEN("isCommand returns false (does not match builtin)")
            {
                CHECK(!result);
            }
        }
    }
}

SCENARIO("isCommand rejects non-commands")
{
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->methodReturns<BuiltinFunctions::list>(
            std::vector<std::string>{"abs", "sqrt", "sin"});
        auto commands = graph.asTrait(trait::commands);
        WHEN("checking \"2 + 3\"")
        {
            auto result = commands.isCommand("2 + 3");

            THEN("isCommand returns false")
            {
                CHECK_FALSE(result);
            }
        }
        WHEN("checking \"x = 5\"")
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
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->methodReturns<BuiltinFunctions::list>(
            std::vector<std::string>{});
        auto commands = graph.asTrait(trait::commands);
        WHEN("calling execute(\"2 + 3\") which is not a command")
        {
            THEN("triggers a contract violation")
            {
                CHECK_THROWS_AS(commands.execute("2 + 3"), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("Clear command clears user functions")
{
    GIVEN("a CommandHandler node with mocked Functions")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        auto commands = graph.asTrait(trait::commands);
        WHEN("executing \"clear\"")
        {
            auto result = commands.execute("clear");

            THEN("Functions.clearUserFunctions is called")
            {
                CHECK(graph.mocks->methodCallCount<Functions::clear>() > 0);
            }
            THEN("Variables.clear is called")
            {
                CHECK(graph.mocks->methodCallCount<Variables::clear>() > 0);
            }
        }
    }
}

SCENARIO("Undef command removes user functions")
{
    GIVEN("a CommandHandler node with mocked Functions")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->enableCallCounting();
        auto commands = graph.asTrait(trait::commands);

        std::vector<std::string> capturedNames;
        graph.mocks->define(
            [&capturedNames](Functions::remove, std::span<std::string const> names) -> std::expected<void, EvalError>
            {
                capturedNames.assign(names.begin(), names.end());
                return {};
            }
        );

        WHEN("executing \"undef f\"")
        {
            auto result = commands.execute("undef f");

            THEN("Functions.removeFunctions is called with [\"f\"]")
            {
                CHECK(result.has_value());
                CHECK(capturedNames.size() == 1);
                CHECK(capturedNames[0] == "f");
            }
        }
    }
}

SCENARIO("Undef command removes multiple functions")
{
    GIVEN("a CommandHandler node with mocked Functions")
    {
        arc::test::Graph<node::CommandHandler> graph;
        graph.mocks->enableCallCounting();
        auto commands = graph.asTrait(trait::commands);

        std::vector<std::string> capturedNames;
        graph.mocks->define(
            [&capturedNames](Functions::remove, std::span<std::string const> names) -> std::expected<void, EvalError>
            {
                capturedNames.assign(names.begin(), names.end());
                return {};
            }
        );

        WHEN("executing \"undef f g\"")
        {
            auto result = commands.execute("undef f g");

            THEN("Functions.removeFunctions is called with [\"f\", \"g\"]")
            {
                CHECK(result.has_value());
                CHECK(capturedNames.size() == 2);
                CHECK(capturedNames[0] == "f");
                CHECK(capturedNames[1] == "g");
            }
        }
    }
}

SCENARIO("Help command mentions function definitions")
{
    GIVEN("a CommandHandler node")
    {
        arc::test::Graph<node::CommandHandler> graph;
        auto commands = graph.asTrait(trait::commands);
        WHEN("executing \"help\"")
        {
            auto result = commands.execute("help");

            THEN("output contains function definition reference")
            {
                CHECK(result.has_value());
                auto output = *result;
                CHECK(output.contains("undef"));
            }
            THEN("output mentions user-defined functions")
            {
                auto output = *result;
                CHECK(output.contains("user-defined function"));
            }
        }
    }
}
