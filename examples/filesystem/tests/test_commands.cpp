import examples.filesystem.command_handler;
import examples.filesystem.tests.mocks;
import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

#include "doctest.h"

using namespace examples::filesystem;

using TestGraph = arc::test::Graph<node::CommandHandler, arc::test::Mock<tests::MockFilesystemTypes>>;

SCENARIO(R"(isCommand recognises known commands)")
{
    GIVEN(R"(a CommandHandler node)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(checking all known commands)")
        {
            THEN(R"(returns true for every known command)")
            {
                CHECK(commands.isCommand("ls"));
                CHECK(commands.isCommand("cat"));
                CHECK(commands.isCommand("write"));
                CHECK(commands.isCommand("mkdir"));
                CHECK(commands.isCommand("rm"));
                CHECK(commands.isCommand("tree"));
                CHECK(commands.isCommand("exists"));
                CHECK(commands.isCommand("help"));
            }

            THEN(R"(returns false for unknown commands)")
            {
                CHECK_FALSE(commands.isCommand("foo"));
                CHECK_FALSE(commands.isCommand(""));
            }
        }
    }
}

SCENARIO(R"(help command lists available commands)")
{
    GIVEN(R"(a CommandHandler node)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "help")")
        {
            std::vector<std::string_view> args{"help"};
            auto result = commands.execute(args);

            THEN(R"(returns success with all commands listed)")
            {
                REQUIRE(result.has_value());
                CHECK(result->contains("ls"));
                CHECK(result->contains("cat"));
                CHECK(result->contains("write"));
                CHECK(result->contains("mkdir"));
                CHECK(result->contains("rm"));
                CHECK(result->contains("tree"));
                CHECK(result->contains("exists"));
                CHECK(result->contains("help"));
                CHECK(result->contains("exit"));
            }
        }
    }
}

SCENARIO(R"(cat command reads file content)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem returning content)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Filesystem::read>(
            std::expected<std::string_view, FsError>{std::string_view{"hello world"}});
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "cat /file.txt")")
        {
            std::vector<std::string_view> args{"cat", "/file.txt"};
            auto result = commands.execute(args);

            THEN(R"(returns file content and reads from the correct path)")
            {
                REQUIRE(result.has_value());
                CHECK(*result == "hello world");

                auto log = graph.mocks->visitCallLogs<Filesystem::read, std::string_view>();
                REQUIRE(log.size() == 1);
                CHECK(std::get<0>(*log.popFront()) == "/file.txt");
            }
        }
    }
}

SCENARIO(R"(cat command propagates filesystem errors)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem returning NotFound)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Filesystem::read>(
            std::expected<std::string_view, FsError>{std::unexpect, FsError::NotFound});
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "cat /nonexistent")")
        {
            std::vector<std::string_view> args{"cat", "/nonexistent"};
            auto result = commands.execute(args);

            THEN(R"(returns error containing "not found")")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("not found"));
            }
        }
    }
}

SCENARIO(R"(write command stores content)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "write /path content")")
        {
            std::vector<std::string_view> args{"write", "/path", "content"};
            auto result = commands.execute(args);

            THEN(R"(returns success and writes to the correct path)")
            {
                REQUIRE(result.has_value());

                auto log = graph.mocks->visitCallLogs<Filesystem::write, std::string_view, std::string>();
                REQUIRE(log.size() == 1);
                auto call = log.popFront();
                REQUIRE(call.has_value());
                CHECK(std::get<0>(*call) == "/path");
                CHECK(std::get<1>(*call) == "content");
            }
        }
    }
}

SCENARIO(R"(write then cat round-trips content)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        std::string storedContent;
        graph.mocks->define(
            [&](trait::Filesystem::write, std::string_view, std::string data) -> std::expected<void, FsError>
            {
                storedContent = std::move(data);
                return {};
            },
            [&](trait::Filesystem::read, std::string_view) -> std::expected<std::string_view, FsError>
            {
                return std::string_view{storedContent};
            });
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(writing then reading back)")
        {
            std::vector<std::string_view> writeArgs{"write", "/file.txt", "hello"};
            auto writeResult = commands.execute(writeArgs);

            std::vector<std::string_view> catArgs{"cat", "/file.txt"};
            auto catResult = commands.execute(catArgs);

            THEN(R"(cat returns the content that was written)")
            {
                REQUIRE(writeResult.has_value());
                REQUIRE(catResult.has_value());
                CHECK(*catResult == "hello");

                auto writeLog = graph.mocks->visitCallLogs<Filesystem::write, std::string_view, std::string>();
                REQUIRE(writeLog.size() == 1);
                auto writeCall = writeLog.popFront();
                REQUIRE(writeCall.has_value());
                CHECK(std::get<0>(*writeCall) == "/file.txt");
                CHECK(std::get<1>(*writeCall) == "hello");

                auto readLog = graph.mocks->visitCallLogs<Filesystem::read, std::string_view>();
                REQUIRE(readLog.size() == 1);
                CHECK(std::get<0>(*readLog.popFront()) == "/file.txt");
            }
        }
    }
}

SCENARIO(R"(write command joins multi-word content)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "write /path word1 word2 word3")")
        {
            std::vector<std::string_view> args{"write", "/path", "word1", "word2", "word3"};
            auto result = commands.execute(args);

            THEN(R"(joins words with spaces)")
            {
                REQUIRE(result.has_value());

                auto log = graph.mocks->visitCallLogs<Filesystem::write, std::string_view, std::string>();
                REQUIRE(log.size() == 1);
                auto call = log.popFront();
                REQUIRE(call.has_value());
                CHECK(std::get<0>(*call) == "/path");
                CHECK(std::get<1>(*call) == "word1 word2 word3");
            }
        }
    }
}

SCENARIO(R"(ls command lists directory contents)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem returning children)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->define(
            [](trait::Filesystem::list, std::string_view) -> std::expected<std::vector<std::string_view>, FsError>
            {
                return std::vector<std::string_view>{"file.txt", "subdir"};
            },
            [](trait::Filesystem::isDir, std::string_view path) -> bool
            {
                return path.contains("subdir") && not path.contains("file");
            });
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "ls /")")
        {
            std::vector<std::string_view> args{"ls", "/"};
            auto result = commands.execute(args);

            THEN(R"(lists the directory contents with directories first)")
            {
                REQUIRE(result.has_value());
                CHECK(result->contains("subdir/"));
                CHECK(result->contains("file.txt"));

                auto listLog = graph.mocks->visitCallLogs<Filesystem::list, std::string_view>();
                REQUIRE(listLog.size() == 1);
                CHECK(std::get<0>(*listLog.popFront()) == "/");
            }
        }

        WHEN(R"(executing "ls" without path defaults to root)")
        {
            std::vector<std::string_view> args{"ls"};
            auto result = commands.execute(args);

            THEN(R"(lists root directory)")
            {
                REQUIRE(result.has_value());

                auto listLog = graph.mocks->visitCallLogs<Filesystem::list, std::string_view>();
                REQUIRE(listLog.size() == 1);
                CHECK(std::get<0>(*listLog.popFront()) == "/");
            }
        }
    }
}

SCENARIO(R"(ls command propagates filesystem errors)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem returning NotFound)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->methodReturns<Filesystem::list>(
            std::expected<std::vector<std::string_view>, FsError>{std::unexpect, FsError::NotFound});
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "ls /nonexistent")")
        {
            std::vector<std::string_view> args{"ls", "/nonexistent"};
            auto result = commands.execute(args);

            THEN(R"(returns error)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("not found"));
            }
        }
    }
}

SCENARIO(R"(mkdir command creates directory)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "mkdir /newdir")")
        {
            std::vector<std::string_view> args{"mkdir", "/newdir"};
            auto result = commands.execute(args);

            THEN(R"(returns success and creates directory at the correct path)")
            {
                REQUIRE(result.has_value());

                auto log = graph.mocks->visitCallLogs<Filesystem::mkdir, std::string_view>();
                REQUIRE(log.size() == 1);
                CHECK(std::get<0>(*log.popFront()) == "/newdir");
            }
        }
    }
}

SCENARIO(R"(rm command removes file)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->logAllCalls();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "rm /file")")
        {
            std::vector<std::string_view> args{"rm", "/file"};
            auto result = commands.execute(args);

            THEN(R"(returns success and removes the correct path)")
            {
                REQUIRE(result.has_value());

                auto log = graph.mocks->visitCallLogs<Filesystem::remove, std::string_view>();
                REQUIRE(log.size() == 1);
                CHECK(std::get<0>(*log.popFront()) == "/file");
            }
        }
    }
}

SCENARIO(R"(tree command displays directory structure)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem returning children)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        graph.mocks->define(
            [](trait::Filesystem::list, std::string_view path) -> std::expected<std::vector<std::string_view>, FsError>
            {
                if (path == "/")
                    return std::vector<std::string_view>{"file.txt", "subdir"};
                return std::vector<std::string_view>{};
            },
            [](trait::Filesystem::isDir, std::string_view path) -> bool
            {
                return path.contains("subdir") && !path.contains("file");
            });
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing "tree")")
        {
            std::vector<std::string_view> args{"tree"};
            auto result = commands.execute(args);

            THEN(R"(returns output with file and directory names)")
            {
                REQUIRE(result.has_value());
                CHECK(result->contains("file.txt"));
                CHECK(result->contains("subdir"));
            }

            THEN(R"(directories are marked with trailing slash)")
            {
                REQUIRE(result.has_value());
                CHECK(result->contains("subdir/"));
            }
        }
    }
}

SCENARIO(R"(exists command checks path existence)")
{
    GIVEN(R"(a CommandHandler with mock Filesystem)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(path exists)")
        {
            graph.mocks->methodReturns<Filesystem::exists>(true);
            std::vector<std::string_view> args{"exists", "/file"};
            auto result = commands.execute(args);

            THEN(R"(returns "true")")
            {
                REQUIRE(result.has_value());
                CHECK(*result == "true");
            }
        }

        WHEN(R"(path does not exist)")
        {
            graph.mocks->methodReturns<Filesystem::exists>(false);
            std::vector<std::string_view> args{"exists", "/missing"};
            auto result = commands.execute(args);

            THEN(R"(returns "false")")
            {
                REQUIRE(result.has_value());
                CHECK(*result == "false");
            }
        }
    }
}

SCENARIO(R"(command with missing args returns usage error)")
{
    GIVEN(R"(a CommandHandler node)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        graph.mocks->methodReturns<Commands::isCommand>(true);
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(executing commands without required arguments)")
        {
            THEN(R"("cat" without path returns usage error)")
            {
                std::vector<std::string_view> args{"cat"};
                auto result = commands.execute(args);
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("Usage:"));
            }

            THEN(R"("write" without path returns usage error)")
            {
                std::vector<std::string_view> args{"write"};
                auto result = commands.execute(args);
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("Usage:"));
            }

            THEN(R"("mkdir" without path returns usage error)")
            {
                std::vector<std::string_view> args{"mkdir"};
                auto result = commands.execute(args);
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("Usage:"));
            }

            THEN(R"("rm" without path returns usage error)")
            {
                std::vector<std::string_view> args{"rm"};
                auto result = commands.execute(args);
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("Usage:"));
            }

            THEN(R"("exists" without path returns usage error)")
            {
                std::vector<std::string_view> args{"exists"};
                auto result = commands.execute(args);
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error().contains("Usage:"));
            }
        }
    }
}

SCENARIO(R"(Commands contract: execute rejects empty args)")
{
    GIVEN(R"(a CommandHandler node)")
    {
        TestGraph graph;
        graph.mocks->setReturnDefault();
        auto commands = graph.asTrait(trait::commands);

        WHEN(R"(calling execute with empty args)")
        {
            std::vector<std::string_view> empty;

            THEN(R"(triggers a contract violation)")
            {
                CHECK_THROWS_AS(commands.execute(empty), arc::ContractViolation);
            }
        }
    }
}
