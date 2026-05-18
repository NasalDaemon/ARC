import examples.filesystem.terminal_line_reader;
import examples.filesystem.tests.mocks;
import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

#include "arc/doctest.h"

using namespace examples::filesystem;

using LineReaderGraph = arc::test::Graph<node::TerminalLineReader, arc::test::Mock<tests::MockFilesystemTypes>>;

SCENARIO(R"(getPathCompletions returns matching entries)")
{
    GIVEN(R"(a LineReader with a filesystem containing /docs/readme.txt and /src/main.cpp)")
    {
        LineReaderGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->define(
            [](trait::Filesystem::list, std::string_view path) -> std::expected<std::vector<std::string_view>, FsError>
            {
                if (path == "/")
                    return std::vector<std::string_view>{"docs", "src"};
                if (path == "/docs")
                    return std::vector<std::string_view>{"readme.txt"};
                if (path == "/src")
                    return std::vector<std::string_view>{"main.cpp"};
                return std::vector<std::string_view>{};
            },
            [](trait::Filesystem::isDir, std::string_view path) -> bool
            {
                return path == "/docs" || path == "/src";
            });

        WHEN(R"(completing "/d")")
        {
            auto completions = graph.node->getPathCompletions("/d");

            THEN(R"(returns "/docs/")")
            {
                REQUIRE(completions.size() == 1);
                CHECK(completions[0] == "/docs/");
            }
        }

        WHEN(R"(completing "/docs/")")
        {
            auto completions = graph.node->getPathCompletions("/docs/");

            THEN(R"(returns "/docs/readme.txt")")
            {
                REQUIRE(completions.size() == 1);
                CHECK(completions[0] == "/docs/readme.txt");
            }
        }

        WHEN(R"(completing "/s")")
        {
            auto completions = graph.node->getPathCompletions("/s");

            THEN(R"(returns "/src/")")
            {
                REQUIRE(completions.size() == 1);
                CHECK(completions[0] == "/src/");
            }
        }

        WHEN(R"(completing "/" at root)")
        {
            auto completions = graph.node->getPathCompletions("/");

            THEN(R"(returns both directories with trailing slash)")
            {
                REQUIRE(completions.size() == 2);
                CHECK(completions[0] == "/docs/");
                CHECK(completions[1] == "/src/");
            }
        }

        WHEN(R"(completing "/nonexistent")")
        {
            auto completions = graph.node->getPathCompletions("/nonexistent");

            THEN(R"(returns empty)")
            {
                CHECK(completions.empty());
            }
        }
    }
}

SCENARIO(R"(getPathCompletions returns common prefix for multiple matches)")
{
    GIVEN(R"(a LineReader with /foo, /foobar, /fizz)")
    {
        LineReaderGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->define(
            [](trait::Filesystem::list, std::string_view path) -> std::expected<std::vector<std::string_view>, FsError>
            {
                if (path == "/")
                    return std::vector<std::string_view>{"foo", "foobar", "fizz"};
                return std::vector<std::string_view>{};
            },
            [](trait::Filesystem::isDir, std::string_view) -> bool
            {
                return false;
            });

        WHEN(R"(completing "/f")")
        {
            auto completions = graph.node->getPathCompletions("/f");

            THEN(R"(returns all three matches)")
            {
                REQUIRE(completions.size() == 3);
                CHECK(completions[0] == "/foo");
                CHECK(completions[1] == "/foobar");
                CHECK(completions[2] == "/fizz");
            }
        }
    }
}
