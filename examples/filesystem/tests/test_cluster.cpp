#include <doctest/doctest.h>

import examples.filesystem.entry;
import examples.filesystem.graphs;
import examples.filesystem.traits;
import arc;
import std;

using namespace examples::filesystem;

TEST_CASE("Integration")
{
    graph::InMemory graph;
    auto fs = graph.asTrait(trait::filesystem);

    SUBCASE("complete workflow")
    {
        // Create directory structure
        REQUIRE(fs.mkdir("/docs").has_value());
        REQUIRE(fs.mkdir("/docs/api").has_value());
        REQUIRE(fs.mkdir("/src").has_value());

        // Create files
        REQUIRE(fs.write("/docs/readme.md", "# README\nWelcome!").has_value());
        REQUIRE(fs.write("/docs/api/spec.json", "{\"version\": \"1.0\"}").has_value());
        REQUIRE(fs.write("/src/main.cpp", "int main() { return 0; }").has_value());

        // Verify structure
        CHECK(fs.exists("/docs"));
        CHECK(fs.exists("/docs/api"));
        CHECK(fs.exists("/src"));
        CHECK(fs.exists("/docs/readme.md"));
        CHECK(fs.exists("/docs/api/spec.json"));
        CHECK(fs.exists("/src/main.cpp"));

        // Read files
        auto readme = fs.read("/docs/readme.md");
        REQUIRE(readme.has_value());
        CHECK(*readme == "# README\nWelcome!");

        // List directories
        auto rootList = fs.list("/");
        REQUIRE(rootList.has_value());
        CHECK(rootList->size() == 2);

        auto docsList = fs.list("/docs");
        REQUIRE(docsList.has_value());
        CHECK(docsList->size() == 2);

        // Remove file
        REQUIRE(fs.remove("/docs/api/spec.json").has_value());
        CHECK_FALSE(fs.exists("/docs/api/spec.json"));

        // Remove empty directory
        REQUIRE(fs.remove("/docs/api").has_value());
        CHECK_FALSE(fs.exists("/docs/api"));

        // Cannot remove non-empty directory
        auto removeNonEmpty = fs.remove("/docs");
        CHECK_FALSE(removeNonEmpty.has_value());
        CHECK(removeNonEmpty.error() == FsError::InvalidPath);
    }

    SUBCASE("path normalization through full stack")
    {
        REQUIRE(fs.mkdir("/test").has_value());
        REQUIRE(fs.write("/test/file.txt", "content").has_value());

        // Access with various path formats
        CHECK(fs.exists("/test/file.txt"));
        CHECK(fs.exists("/test/./file.txt"));
        CHECK(fs.exists("/test/../test/file.txt"));

        auto content = fs.read("/./test/../test/file.txt");
        REQUIRE(content.has_value());
        CHECK(*content == "content");
    }

    SUBCASE("error propagation through stack")
    {
        // Read nonexistent
        CHECK(fs.read("/nonexistent").error() == FsError::NotFound);

        // Write to nonexistent parent
        CHECK(fs.write("/nonexistent/file.txt", "data").error() == FsError::NotFound);

        // Create file then try to treat as directory
        REQUIRE(fs.write("/file.txt", "data").has_value());
        CHECK(fs.list("/file.txt").error() == FsError::NotADirectory);
        CHECK(fs.write("/file.txt/nested.txt", "data").error() == FsError::NotADirectory);

        // Create directory then try to read as file
        REQUIRE(fs.mkdir("/dir").has_value());
        CHECK(fs.read("/dir").error() == FsError::IsADirectory);

        // Try to create existing directory
        CHECK(fs.mkdir("/dir").error() == FsError::AlreadyExists);
    }
}
