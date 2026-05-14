import examples.calculator.node.repl;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#include "doctest.h"

using namespace examples::calculator;

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
            graph.node->run();

            THEN("Output::writeLine is never called")
            {
                CHECK(graph.mocks->methodCallCount<Output::writeLine>() == 0);
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
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>("quit"));

        WHEN("the first line is \"quit\"")
        {
            auto rc = graph.node->run();

            THEN("run() returns 0")
            {
                CHECK(rc == 0);
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
        graph.mocks->methodReturns<LineReader::readLine>(
            std::optional<std::string>("exit"));

        WHEN("the first line is \"exit\"")
        {
            auto rc = graph.node->run();

            THEN("run() returns 0")
            {
                CHECK(rc == 0);
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
            graph.node->run();

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
        graph.mocks->define(
            [](Formatter::formatError, std::string_view msg) -> std::string
            {
                return std::format("Error: {}", msg);
            }
        );

        WHEN("command returns an error")
        {
            graph.node->run();

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

SCENARIO("Tokeniser error is displayed via formatError")
{
    GIVEN("a Repl node with invalid input")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(false);
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "@#!", std::nullopt);
        graph.mocks->define(
            [](Tokeniser::tokenise, std::string_view) -> std::expected<std::vector<Token>, ParseError>
            {
                return std::unexpected(ParseError{"bad token", 0});
            },
            [](Formatter::formatError, std::string_view msg) -> std::string
            {
                return std::format("Error: {}", msg);
            }
        );

        WHEN("the tokeniser returns an error")
        {
            graph.node->run();

            THEN("the error message is written to output")
            {
                auto visitor = graph.mocks->visitCallLogs<Output::writeLine, std::string_view>();
                REQUIRE(visitor.size() >= 1);
                auto last = visitor.back();
                REQUIRE(last.has_value());
                CHECK(std::get<0>(*last).contains("bad token"));
            }
        }
    }
}

SCENARIO("Evaluator error is displayed via formatError")
{
    GIVEN("a Repl node where evaluation fails")
    {
        arc::test::Graph<node::Repl> graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(false);
        graph.mocks->methodReturns<Evaluator::evaluate>(
            std::expected<EvalResult, EvalError>{std::unexpect, EvalError{"division by zero"}});
        graph.mocks->methodReturnsN<LineReader::readLine, std::optional<std::string>>(
            "1 / 0", std::nullopt);
        graph.mocks->define(
            [](Tokeniser::tokenise, std::string_view input) -> std::expected<std::vector<Token>, ParseError>
            {
                return std::vector<Token>{{TokenType::Number, input, 1.0}, {TokenType::End, {}}};
            },
            [](Parser::parse, std::span<Token const>, std::string_view) -> std::expected<ExprPtr, ParseError>
            {
                return std::make_unique<Expression>(NumberExpr{1.0});
            },
            [](Formatter::formatError, std::string_view msg) -> std::string
            {
                return std::format("Error: {}", msg);
            }
        );

        WHEN("the evaluator returns an error")
        {
            graph.node->run();

            THEN("the error is written to output")
            {
                auto visitor = graph.mocks->visitCallLogs<Output::writeLine, std::string_view>();
                REQUIRE(visitor.size() >= 1);
                auto last = visitor.back();
                REQUIRE(last.has_value());
                CHECK(std::get<0>(*last).contains("division by zero"));
            }
        }
    }
}
