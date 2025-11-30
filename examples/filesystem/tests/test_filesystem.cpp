#include <doctest/doctest.h>

import examples.filesystem.entry;
import examples.filesystem.graphs;
import examples.filesystem.traits;
import arc;
import std;

using namespace examples::filesystem;

TEST_CASE("Filesystem")
{
    InMemoryGraph graph;
    auto fs = graph.fs.asTrait(trait::filesystem);

    SUBCASE("mkdir creates directory")
    {
        auto result = fs.mkdir("/docs");
        REQUIRE(result.has_value());

        CHECK(fs.exists("/docs"));
        CHECK(fs.isDir("/docs"));
    }

    SUBCASE("mkdir fails if parent doesn't exist")
    {
        auto result = fs.mkdir("/nonexistent/subdir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("mkdir fails if parent is a file")
    {
        REQUIRE(fs.write("/file.txt", "content").has_value());

        auto result = fs.mkdir("/file.txt/subdir");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("mkdir fails if already exists")
    {
        REQUIRE(fs.mkdir("/docs2").has_value());

        auto result = fs.mkdir("/docs2");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::AlreadyExists);
    }

    SUBCASE("write creates file")
    {
        auto result = fs.write("/test.txt", "hello world");
        REQUIRE(result.has_value());

        CHECK(fs.exists("/test.txt"));
        CHECK_FALSE(fs.isDir("/test.txt"));
    }

    SUBCASE("write overwrites existing file")
    {
        REQUIRE(fs.write("/file2.txt", "first").has_value());
        REQUIRE(fs.write("/file2.txt", "second").has_value());

        auto content = fs.read("/file2.txt");
        REQUIRE(content.has_value());
        CHECK(*content == "second");
    }

    SUBCASE("write fails if parent doesn't exist")
    {
        auto result = fs.write("/nonexistent/file.txt", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("write fails if parent is a file")
    {
        REQUIRE(fs.write("/file3.txt", "content").has_value());

        auto result = fs.write("/file3.txt/nested.txt", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("write fails if path is a directory")
    {
        REQUIRE(fs.mkdir("/docs3").has_value());

        auto result = fs.write("/docs3", "data");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::IsADirectory);
    }

    SUBCASE("read returns file content")
    {
        REQUIRE(fs.write("/test2.txt", "hello world").has_value());

        auto content = fs.read("/test2.txt");
        REQUIRE(content.has_value());
        CHECK(*content == "hello world");
    }

    SUBCASE("read fails for nonexistent file")
    {
        auto result = fs.read("/nonexistent.txt");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("read fails for directory")
    {
        REQUIRE(fs.mkdir("/docs4").has_value());

        auto result = fs.read("/docs4");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::IsADirectory);
    }

    SUBCASE("list returns directory contents")
    {
        REQUIRE(fs.mkdir("/docs5").has_value());
        REQUIRE(fs.write("/docs5/a.txt", "a").has_value());
        REQUIRE(fs.write("/docs5/b.txt", "b").has_value());

        auto entries = fs.list("/docs5");
        REQUIRE(entries.has_value());
        CHECK(entries->size() == 2);
        CHECK((*entries)[0] == "a.txt");
        CHECK((*entries)[1] == "b.txt");
    }

    SUBCASE("list fails for nonexistent path")
    {
        auto result = fs.list("/nonexistent");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotFound);
    }

    SUBCASE("list fails for file")
    {
        REQUIRE(fs.write("/file4.txt", "content").has_value());

        auto result = fs.list("/file4.txt");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::NotADirectory);
    }

    SUBCASE("remove deletes file")
    {
        REQUIRE(fs.write("/file5.txt", "content").has_value());

        auto result = fs.remove("/file5.txt");
        REQUIRE(result.has_value());
        CHECK_FALSE(fs.exists("/file5.txt"));
    }

    SUBCASE("remove deletes empty directory")
    {
        REQUIRE(fs.mkdir("/empty").has_value());

        auto result = fs.remove("/empty");
        REQUIRE(result.has_value());
        CHECK_FALSE(fs.exists("/empty"));
    }

    SUBCASE("remove fails for non-empty directory")
    {
        REQUIRE(fs.mkdir("/docs6").has_value());
        REQUIRE(fs.write("/docs6/file.txt", "content").has_value());

        auto result = fs.remove("/docs6");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::InvalidPath);
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

    SUBCASE("exists returns correct values")
    {
        CHECK(fs.exists("/"));
        CHECK_FALSE(fs.exists("/nonexistent"));

        REQUIRE(fs.mkdir("/docs7").has_value());
        CHECK(fs.exists("/docs7"));

        REQUIRE(fs.write("/file6.txt", "content").has_value());
        CHECK(fs.exists("/file6.txt"));
    }

    SUBCASE("isDir returns correct values")
    {
        CHECK(fs.isDir("/"));

        REQUIRE(fs.mkdir("/docs8").has_value());
        CHECK(fs.isDir("/docs8"));

        REQUIRE(fs.write("/file7.txt", "content").has_value());
        CHECK_FALSE(fs.isDir("/file7.txt"));
        CHECK_FALSE(fs.isDir("/nonexistent"));
    }

    SUBCASE("path normalization is applied")
    {
        REQUIRE(fs.mkdir("/docs9").has_value());
        REQUIRE(fs.write("/docs9/file.txt", "content").has_value());

        // Various path formats should work
        CHECK(fs.exists("/docs9/./file.txt"));
        CHECK(fs.exists("/docs9/../docs9/file.txt"));

        auto content = fs.read("/./docs9/../docs9/file.txt");
        REQUIRE(content.has_value());
        CHECK(*content == "content");
    }
}
