import examples.filesystem.tests.mocks;
import examples.filesystem.filesystem;
import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

#include "doctest.h"

namespace examples::filesystem::tests {

SCENARIO(R"(Filesystem node operations with mocks)")
{
    GIVEN(R"(a filesystem with mocked dependencies)")
    {
        arc::test::Graph<node::Filesystem, arc::test::Mock<MockStorageTypes>> graph;
        auto fs = graph.asTrait(trait::filesystem);

        graph.mocks->setThrowIfMissing();
        MockStorage storage(graph);

        graph.mocks->define(
            [](trait::PathOps::normalise, std::string_view path) {
                return std::string(path);
            },
            [](trait::PathOps::parent, std::string_view path) {
                auto pos = path.rfind('/');
                return pos == 0 || pos == std::string_view::npos
                    ? "/"
                    : std::string(path.substr(0, pos));
            },
            [](trait::PathOps::isRoot, std::string_view path) {
                return path == "/";
            }
        );

        WHEN(R"(reading a nonexistent path)")
        {
            auto result = fs.read("/nonexistent");

            THEN(R"(it returns NotFound)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotFound);
            }
        }

        WHEN(R"(reading a directory)")
        {
            storage.put("/dir", Entry::directory());

            auto result = fs.read("/dir");

            THEN(R"(it returns IsADirectory)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::IsADirectory);
            }
        }

        WHEN(R"(reading an existing file)")
        {
            storage.put("/file.txt", Entry::file("hello world"));

            auto result = fs.read("/file.txt");

            THEN(R"(it returns the file content)")
            {
                REQUIRE(result.has_value());
                CHECK(*result == "hello world");
            }
        }

        WHEN(R"(writing to a path whose parent doesn't exist)")
        {
            auto result = fs.write("/nonexistent/file.txt", "data");

            THEN(R"(it returns NotFound)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotFound);
            }
        }

        WHEN(R"(writing to a path whose parent is not a directory)")
        {
            storage.put("/file", Entry::file("content"));

            auto result = fs.write("/file/nested.txt", "data");

            THEN(R"(it returns NotADirectory)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotADirectory);
            }
        }

        WHEN(R"(writing to a path that is an existing directory)")
        {
            storage.put("/dir", Entry::directory());

            auto result = fs.write("/dir", "data");

            THEN(R"(it returns IsADirectory)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::IsADirectory);
            }
        }

        WHEN(R"(writing to a valid parent)")
        {
            auto result = fs.write("/newfile.txt", "content");

            THEN(R"(it creates the file)")
            {
                REQUIRE(result.has_value());

                auto* entry = storage.get("/newfile.txt");
                REQUIRE(entry != nullptr);
                CHECK_FALSE(entry->isDir());
                CHECK(entry->content() == "content");
            }
        }

        WHEN(R"(mkdir at a path whose parent doesn't exist)")
        {
            auto result = fs.mkdir("/nonexistent/subdir");

            THEN(R"(it returns NotFound)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotFound);
            }
        }

        WHEN(R"(mkdir at a path whose parent is not a directory)")
        {
            storage.put("/file", Entry::file("content"));

            auto result = fs.mkdir("/file/subdir");

            THEN(R"(it returns NotADirectory)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotADirectory);
            }
        }

        WHEN(R"(mkdir at a path that already exists)")
        {
            storage.put("/existing", Entry::directory());

            auto result = fs.mkdir("/existing");

            THEN(R"(it returns AlreadyExists)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::AlreadyExists);
            }
        }

        WHEN(R"(mkdir at root)")
        {
            auto result = fs.mkdir("/");

            THEN(R"(it returns AlreadyExists)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::AlreadyExists);
            }
        }

        WHEN(R"(mkdir at a valid parent)")
        {
            auto result = fs.mkdir("/newdir");

            THEN(R"(it creates the directory)")
            {
                REQUIRE(result.has_value());

                auto* entry = storage.get("/newdir");
                REQUIRE(entry != nullptr);
                CHECK(entry->isDir());
            }
        }

        WHEN(R"(removing a nonexistent path)")
        {
            auto result = fs.remove("/nonexistent");

            THEN(R"(it returns NotFound)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotFound);
            }
        }

        WHEN(R"(removing root)")
        {
            auto result = fs.remove("/");

            THEN(R"(it returns InvalidPath)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::InvalidPath);
            }
        }

        WHEN(R"(removing a non-empty directory)")
        {
            storage.put("/dir", Entry::directory());
            storage.childrenMap["/dir"] = {"child"};

            auto result = fs.remove("/dir");

            THEN(R"(it returns NotEmpty)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotEmpty);
            }
        }

        WHEN(R"(removing an existing file)")
        {
            storage.put("/file.txt", Entry::file("content"));

            auto result = fs.remove("/file.txt");

            THEN(R"(it deletes the file)")
            {
                REQUIRE(result.has_value());
                CHECK(storage.get("/file.txt") == nullptr);
            }
        }

        WHEN(R"(removing an empty directory)")
        {
            storage.put("/emptydir", Entry::directory());
            storage.childrenMap["/emptydir"] = {};

            auto result = fs.remove("/emptydir");

            THEN(R"(it deletes the directory)")
            {
                REQUIRE(result.has_value());
                CHECK(storage.get("/emptydir") == nullptr);
            }
        }

        WHEN(R"(listing a nonexistent path)")
        {
            auto result = fs.list("/nonexistent");

            THEN(R"(it returns NotFound)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotFound);
            }
        }

        WHEN(R"(listing a file)")
        {
            storage.put("/file.txt", Entry::file("content"));

            auto result = fs.list("/file.txt");

            THEN(R"(it returns NotADirectory)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::NotADirectory);
            }
        }

        WHEN(R"(listing a directory)")
        {
            storage.put("/dir", Entry::directory());
            storage.childrenMap["/dir"] = {"a.txt", "b.txt"};

            auto result = fs.list("/dir");

            THEN(R"(it returns the directory children)")
            {
                REQUIRE(result.has_value());
                CHECK(result->size() == 2);
            }
        }

        WHEN(R"(checking existence of a nonexistent path)")
        {
            THEN(R"(exists returns false)")
            {
                CHECK_FALSE(fs.exists("/nonexistent"));
            }
        }

        WHEN(R"(checking existence of a file)")
        {
            storage.put("/file.txt", Entry::file("content"));

            THEN(R"(exists returns true)")
            {
                CHECK(fs.exists("/file.txt"));
            }
        }

        WHEN(R"(checking existence of a directory)")
        {
            storage.put("/dir", Entry::directory());

            THEN(R"(exists returns true)")
            {
                CHECK(fs.exists("/dir"));
            }
        }

        WHEN(R"(checking isDir on a nonexistent path)")
        {
            THEN(R"(isDir returns false)")
            {
                CHECK_FALSE(fs.isDir("/nonexistent"));
            }
        }

        WHEN(R"(checking isDir on a file)")
        {
            storage.put("/file.txt", Entry::file("content"));

            THEN(R"(isDir returns false)")
            {
                CHECK_FALSE(fs.isDir("/file.txt"));
            }
        }

        WHEN(R"(checking isDir on a directory)")
        {
            storage.put("/dir", Entry::directory());

            THEN(R"(isDir returns true)")
            {
                CHECK(fs.isDir("/dir"));
            }
        }
    }
}

SCENARIO(R"(Filesystem node uses PathOps for normalization)")
{
    GIVEN(R"(a filesystem with path-transforming mocks)")
    {
        arc::test::Graph<node::Filesystem, arc::test::Mock<MockStorageTypes>> graph;
        auto fs = graph.asTrait(trait::filesystem);

        graph.mocks->setThrowIfMissing();

        MockStorage storage(graph);

        graph.mocks->define(
            [](trait::PathOps::normalise, std::string_view path) -> std::string {
                if (path == "/docs/../file.txt") return "/file.txt";
                return std::string(path);
            },
            [](trait::PathOps::parent, std::string_view path) -> std::string {
                if (path == "/file.txt") return "/";
                auto pos = path.rfind('/');
                if (pos == 0 || pos == std::string_view::npos) return "/";
                return std::string(path.substr(0, pos));
            },
            [](trait::PathOps::isRoot, std::string_view path) -> bool {
                return path == "/";
            }
        );

        WHEN(R"(writing to a path with parent traversal)")
        {
            auto result = fs.write("/docs/../file.txt", "content");

            THEN(R"(it uses the normalized path)")
            {
                REQUIRE(result.has_value());
                CHECK(storage.get("/file.txt") != nullptr);
                CHECK(storage.get("/docs/../file.txt") == nullptr);
            }
        }

        WHEN(R"(enabling call counting and calling exists)")
        {
            graph.mocks->enableCallCounting();

            THEN(R"(PathOps::normalise is called exactly once)")
            {
                CHECK(graph.mocks->methodCallCount<trait::PathOps::normalise>() == 0);
                fs.exists("/some/path");
                CHECK(graph.mocks->methodCallCount<trait::PathOps::normalise>() == 1);
            }
        }
    }
}

SCENARIO(R"(Filesystem trait methods reject empty paths)")
{
    arc::test::Graph<node::Filesystem, arc::test::Mock<MockStorageTypes>> graph;
    auto fs = graph.asTrait(trait::filesystem);

    graph.mocks->setThrowIfMissing();
    MockStorage storage(graph);

    graph.mocks->define(
        [](trait::PathOps::normalise, std::string_view path) {
            return std::string(path);
        },
        [](trait::PathOps::parent, std::string_view path) {
            auto pos = path.rfind('/');
            return pos == 0 || pos == std::string_view::npos
                ? "/"
                : std::string(path.substr(0, pos));
        },
        [](trait::PathOps::isRoot, std::string_view path) {
            return path == "/";
        }
    );

    GIVEN(R"(a filesystem with mocked dependencies)")
    {
        WHEN(R"(calling read with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.read(""), arc::ContractViolation);
            }
        }

        WHEN(R"(calling write with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.write("", "data"), arc::ContractViolation);
            }
        }

        WHEN(R"(calling mkdir with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.mkdir(""), arc::ContractViolation);
            }
        }

        WHEN(R"(calling remove with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.remove(""), arc::ContractViolation);
            }
        }

        WHEN(R"(calling list with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.list(""), arc::ContractViolation);
            }
        }

        WHEN(R"(calling exists with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.exists(""), arc::ContractViolation);
            }
        }

        WHEN(R"(calling isDir with an empty path)")
        {
            THEN(R"(it triggers a contract violation)")
            {
                CHECK_THROWS_AS(fs.isDir(""), arc::ContractViolation);
            }
        }
    }
}

} // namespace examples::filesystem::tests
