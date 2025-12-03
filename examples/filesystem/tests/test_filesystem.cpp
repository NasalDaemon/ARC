#include <doctest/doctest.h>

import examples.filesystem.tests.mocks;
import examples.filesystem.filesystem;
import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

namespace examples::filesystem::tests {

TEST_CASE("Filesystem node with mocks")
{
    // Create test graph with Filesystem node and mocked dependencies
    // The Mock provides Types that Storage trait needs
    arc::test::Graph<Filesystem, arc::test::Mock<MockStorageTypes>> graph;
    auto fs = graph.asTrait(trait::filesystem);

    // Set up mock behavior
    graph.mocks->setThrowIfMissing();
    MockStorage storage;

    // Define PathOps mock - just returns the path as-is for simplicity
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

    // Define Storage mock
    graph.mocks->define(
        [&](trait::Storage::get, std::string_view path) {
            return storage.get(path);
        },
        [&](trait::Storage::put, std::string_view path, InMemoryEntry entry) {
            return storage.put(path, std::move(entry));
        },
        [&](trait::Storage::erase, std::string_view path) {
            return storage.erase(path);
        },
        [&](trait::Storage::children, std::string_view path) {
            return storage.children(path);
        }
    );

    SUBCASE("read returns NotFound for nonexistent path")
    {
        auto result = fs.read("/nonexistent");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("read returns IsADirectory for directory")
    {
        storage.put("/dir", InMemoryEntry::directory());

        auto result = fs.read("/dir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::IsADirectory);
    }

    SUBCASE("read returns file content")
    {
        storage.put("/file.txt", InMemoryEntry::file("hello world"));

        auto result = fs.read("/file.txt");
        REQUIRE(result.has_value());
        CHECK(*result == "hello world");
    }

    SUBCASE("write fails if parent doesn't exist")
    {
        auto result = fs.write("/nonexistent/file.txt", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("write fails if parent is not a directory")
    {
        storage.put("/file", InMemoryEntry::file("content"));

        auto result = fs.write("/file/nested.txt", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("write fails if target is a directory")
    {
        storage.put("/dir", InMemoryEntry::directory());

        auto result = fs.write("/dir", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::IsADirectory);
    }

    SUBCASE("write creates file in valid parent")
    {
        auto result = fs.write("/newfile.txt", "content");
        REQUIRE(result.has_value());

        auto* entry = storage.get("/newfile.txt");
        REQUIRE(entry != nullptr);
        CHECK_FALSE(entry->isDir());
        CHECK(entry->content() == "content");
    }

    SUBCASE("mkdir fails if parent doesn't exist")
    {
        auto result = fs.mkdir("/nonexistent/subdir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("mkdir fails if parent is not a directory")
    {
        storage.put("/file", InMemoryEntry::file("content"));

        auto result = fs.mkdir("/file/subdir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("mkdir fails if already exists")
    {
        storage.put("/existing", InMemoryEntry::directory());

        auto result = fs.mkdir("/existing");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::AlreadyExists);
    }

    SUBCASE("mkdir fails for root")
    {
        auto result = fs.mkdir("/");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::AlreadyExists);
    }

    SUBCASE("mkdir creates directory in valid parent")
    {
        auto result = fs.mkdir("/newdir");
        REQUIRE(result.has_value());

        auto* entry = storage.get("/newdir");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir());
    }

    SUBCASE("remove fails for nonexistent path")
    {
        auto result = fs.remove("/nonexistent");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("remove fails for root")
    {
        auto result = fs.remove("/");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::InvalidPath);
    }

    SUBCASE("remove fails for non-empty directory")
    {
        storage.put("/dir", InMemoryEntry::directory());
        storage.childrenMap["/dir"] = {"child"};

        auto result = fs.remove("/dir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::InvalidPath);
    }

    SUBCASE("remove deletes file")
    {
        storage.put("/file.txt", InMemoryEntry::file("content"));

        auto result = fs.remove("/file.txt");
        REQUIRE(result.has_value());
        CHECK(storage.get("/file.txt") == nullptr);
    }

    SUBCASE("remove deletes empty directory")
    {
        storage.put("/emptydir", InMemoryEntry::directory());
        storage.childrenMap["/emptydir"] = {};

        auto result = fs.remove("/emptydir");
        REQUIRE(result.has_value());
        CHECK(storage.get("/emptydir") == nullptr);
    }

    SUBCASE("list fails for nonexistent path")
    {
        auto result = fs.list("/nonexistent");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("list fails for file")
    {
        storage.put("/file.txt", InMemoryEntry::file("content"));

        auto result = fs.list("/file.txt");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("list returns directory children")
    {
        storage.put("/dir", InMemoryEntry::directory());
        storage.childrenMap["/dir"] = {"a.txt", "b.txt"};

        auto result = fs.list("/dir");
        REQUIRE(result.has_value());
        CHECK(result->size() == 2);
    }

    SUBCASE("exists returns false for nonexistent")
    {
        CHECK_FALSE(fs.exists("/nonexistent"));
    }

    SUBCASE("exists returns true for file")
    {
        storage.put("/file.txt", InMemoryEntry::file("content"));
        CHECK(fs.exists("/file.txt"));
    }

    SUBCASE("exists returns true for directory")
    {
        storage.put("/dir", InMemoryEntry::directory());
        CHECK(fs.exists("/dir"));
    }

    SUBCASE("isDir returns false for nonexistent")
    {
        CHECK_FALSE(fs.isDir("/nonexistent"));
    }

    SUBCASE("isDir returns false for file")
    {
        storage.put("/file.txt", InMemoryEntry::file("content"));
        CHECK_FALSE(fs.isDir("/file.txt"));
    }

    SUBCASE("isDir returns true for directory")
    {
        storage.put("/dir", InMemoryEntry::directory());
        CHECK(fs.isDir("/dir"));
    }
}

TEST_CASE("Filesystem node uses PathOps for normalization")
{
    arc::test::Graph<Filesystem, arc::test::Mock<MockStorageTypes>> graph;
    auto fs = graph.asTrait(trait::filesystem);

    graph.mocks->setThrowIfMissing();

    MockStorage storage;

    // PathOps mock that transforms paths
    graph.mocks->define(
        [](trait::PathOps::normalise, std::string_view path) -> std::string {
            // Simulate normalization: /a/../b -> /b
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

    graph.mocks->define(
        [&](trait::Storage::get, std::string_view path) -> InMemoryEntry const* {
            return storage.get(path);
        },
        [&](trait::Storage::put, std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError> {
            return storage.put(path, std::move(entry));
        },
        [&](trait::Storage::erase, std::string_view path) -> bool {
            return storage.erase(path);
        },
        [&](trait::Storage::children, std::string_view path) -> std::vector<std::string_view> {
            return storage.children(path);
        }
    );

    SUBCASE("write uses normalized path")
    {
        auto result = fs.write("/docs/../file.txt", "content");
        REQUIRE(result.has_value());

        // Storage should receive the normalized path
        CHECK(storage.get("/file.txt") != nullptr);
        CHECK(storage.get("/docs/../file.txt") == nullptr);
    }

    SUBCASE("PathOps::normalise is called")
    {
        graph.mocks->enableCallCounting();
        CHECK(graph.mocks->methodCallCount<trait::PathOps::normalise>() == 0);
        fs.exists("/some/path");
        CHECK(graph.mocks->methodCallCount<trait::PathOps::normalise>() == 1);
    }
}

} // namespace examples::filesystem::tests
