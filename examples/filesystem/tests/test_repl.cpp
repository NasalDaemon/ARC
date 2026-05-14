import examples.filesystem.repl;
import examples.filesystem.traits;
import arc;
import std;

#include "doctest.h"

using namespace examples::filesystem;

SCENARIO("Empty input line is skipped")
{
    GIVEN("a Repl node")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "", std::nullopt);

        WHEN("the first line is empty and then EOF")
        {
            graph.node->run(0, nullptr);

            THEN("only the banner is printed, no extra output")
            {
                // Banner has 4 writeLine calls; empty input should add none
                CHECK(graph.mocks->methodCallCount<Output::writeLine>() == 4);
            }
        }
    }
}

SCENARIO("\"quit\" terminates the loop")
{
    GIVEN("a Repl node")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>("quit"));

        WHEN("the first line is \"quit\"")
        {
            graph.node->run(0, nullptr);

            THEN("the REPL exits cleanly")
            {
                CHECK(graph.mocks->methodCallCount<LineReader::readLine>() == 1);
            }
        }
    }
}

SCENARIO("\"exit\" terminates the loop")
{
    GIVEN("a Repl node")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>("exit"));

        WHEN("the first line is \"exit\"")
        {
            graph.node->run(0, nullptr);

            THEN("the REPL exits cleanly")
            {
                CHECK(graph.mocks->methodCallCount<LineReader::readLine>() == 1);
            }
        }
    }
}

SCENARIO("\"q\" terminates the loop")
{
    GIVEN("a Repl node")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>("q"));

        WHEN("the first line is \"q\"")
        {
            graph.node->run(0, nullptr);

            THEN("the REPL exits cleanly")
            {
                CHECK(graph.mocks->methodCallCount<LineReader::readLine>() == 1);
            }
        }
    }
}

SCENARIO("Commands are dispatched to CommandHandler")
{
    GIVEN("a Repl node with a command input")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Commands::execute>(
            std::expected<std::string, std::string>{std::string{"ok"}});
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "help", std::nullopt);

        WHEN("executing \"help\"")
        {
            graph.node->run(0, nullptr);

            THEN("Commands::execute is called once")
            {
                CHECK(graph.mocks->methodCallCount<Commands::execute>() == 1);
            }
        }
    }
}

SCENARIO("Command error is formatted and displayed")
{
    GIVEN("a Repl node with a failing command")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Commands::execute>(
            std::expected<std::string, std::string>{std::unexpect, std::string{"something went wrong"}});
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "badcmd", std::nullopt);

        WHEN("command returns an error")
        {
            graph.node->run(0, nullptr);

            THEN("the formatted error is written to output")
            {
                auto visitor = graph.mocks->visitCallLogs<Output::writeLine, std::string_view>();
                REQUIRE(visitor.size() >= 1);
                auto last = visitor.back();
                REQUIRE(last.has_value());
                CHECK(std::get<0>(*last).contains("something went wrong"));
            }
        }
    }
}

SCENARIO("Unknown command is reported")
{
    GIVEN("a Repl node with an unknown command")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(false);
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "foo", std::nullopt);

        WHEN("the command is not recognised")
        {
            graph.node->run(0, nullptr);

            THEN("\"Unknown command: foo\" is written to output")
            {
                auto visitor = graph.mocks->visitCallLogs<Output::writeLine, std::string_view>();
                REQUIRE(visitor.size() >= 1);
                auto last = visitor.back();
                REQUIRE(last.has_value());
                CHECK(std::get<0>(*last) == "Unknown command: foo");
            }
        }
    }
}

SCENARIO("CLI args are joined and executed")
{
    GIVEN("a Repl node with CLI args")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Commands::execute>(
            std::expected<std::string, std::string>{std::string{"ok"}});

        WHEN("run is called with \"help\" and \"exit\" args")
        {
            char arg0[] = "repl";
            char arg1[] = "help";
            char arg2[] = "exit";
            char* args[] = {arg0, arg1, arg2};
            graph.node->run(3, args);

            THEN("Commands::execute is called once for the joined args")
            {
                CHECK(graph.mocks->methodCallCount<Commands::execute>() == 1);
            }
        }
    }
}

SCENARIO("EOF terminates the loop")
{
    GIVEN("a Repl node")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->enableCallCounting();
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>{std::nullopt});

        WHEN("readLine returns EOF immediately")
        {
            graph.node->run(0, nullptr);

            THEN("the REPL exits after one read attempt")
            {
                CHECK(graph.mocks->methodCallCount<LineReader::readLine>() == 1);
            }

            THEN("only the banner is printed")
            {
                CHECK(graph.mocks->methodCallCount<Output::writeLine>() == 4);
            }
        }
    }
}
