import examples.filesystem.types;
import examples.filesystem.graphs;
import examples.filesystem.traits;
import arc;
import std;

#include "doctest.h"

using namespace examples::filesystem;

SCENARIO(R"(filesystem integration tests)")
{
    GIVEN(R"(an in-memory filesystem graph)")
    {
        graph::InMemory graph;
        auto fs = graph.asTrait(trait::filesystem);

        WHEN(R"(performing a complete workflow)")
        {
            // Create directory structure
            REQUIRE(fs.mkdir("/docs").has_value());
            REQUIRE(fs.mkdir("/docs/api").has_value());
            REQUIRE(fs.mkdir("/src").has_value());

            // Create files
            REQUIRE(fs.write("/docs/readme.md", "# README\nWelcome!").has_value());
            REQUIRE(fs.write("/docs/api/spec.json", "{\"version\": \"1.0\"}").has_value());
            REQUIRE(fs.write("/src/main.cpp", "int main() { return 0; }").has_value());

            THEN(R"(the directory structure exists)")
            {
                CHECK(fs.exists("/docs"));
                CHECK(fs.exists("/docs/api"));
                CHECK(fs.exists("/src"));
                CHECK(fs.exists("/docs/readme.md"));
                CHECK(fs.exists("/docs/api/spec.json"));
                CHECK(fs.exists("/src/main.cpp"));
            }

            THEN(R"(files can be read back)")
            {
                auto readme = fs.read("/docs/readme.md");
                REQUIRE(readme.has_value());
                CHECK(*readme == "# README\nWelcome!");
            }

            THEN(R"(directories can be listed)")
            {
                auto rootList = fs.list("/");
                REQUIRE(rootList.has_value());
                CHECK(rootList->size() == 2);

                auto docsList = fs.list("/docs");
                REQUIRE(docsList.has_value());
                CHECK(docsList->size() == 2);
            }

            THEN(R"(files and empty directories can be removed)")
            {
                REQUIRE(fs.remove("/docs/api/spec.json").has_value());
                CHECK_FALSE(fs.exists("/docs/api/spec.json"));

                REQUIRE(fs.remove("/docs/api").has_value());
                CHECK_FALSE(fs.exists("/docs/api"));
            }

            THEN(R"(non-empty directories cannot be removed)")
            {
                auto removeNonEmpty = fs.remove("/docs");
                CHECK_FALSE(removeNonEmpty.has_value());
                CHECK(removeNonEmpty.error() == FsError::NotEmpty);
            }
        }

        WHEN(R"(normalizing paths through the full stack)")
        {
            REQUIRE(fs.mkdir("/test").has_value());
            REQUIRE(fs.write("/test/file.txt", "content").has_value());

            THEN(R"(files are accessible with various path formats)")
            {
                CHECK(fs.exists("/test/file.txt"));
                CHECK(fs.exists("/test/./file.txt"));
                CHECK(fs.exists("/test/../test/file.txt"));
            }

            THEN(R"(content can be read through normalized paths)")
            {
                auto content = fs.read("/./test/../test/file.txt");
                REQUIRE(content.has_value());
                CHECK(*content == "content");
            }
        }

        WHEN(R"(propagating errors through the stack)")
        {
            THEN(R"(reading a nonexistent path returns NotFound)")
            {
                CHECK(fs.read("/nonexistent").error() == FsError::NotFound);
            }

            THEN(R"(writing to a nonexistent parent returns NotFound)")
            {
                CHECK(fs.write("/nonexistent/file.txt", "data").error() == FsError::NotFound);
            }

            THEN(R"(treating a file as a directory returns NotADirectory)")
            {
                REQUIRE(fs.write("/file.txt", "data").has_value());
                CHECK(fs.list("/file.txt").error() == FsError::NotADirectory);
                CHECK(fs.write("/file.txt/nested.txt", "data").error() == FsError::NotADirectory);
            }

            THEN(R"(treating a directory as a file returns IsADirectory)")
            {
                REQUIRE(fs.mkdir("/dir").has_value());
                CHECK(fs.read("/dir").error() == FsError::IsADirectory);
            }

            THEN(R"(creating an existing directory returns AlreadyExists)")
            {
                REQUIRE(fs.mkdir("/dir").has_value());
                CHECK(fs.mkdir("/dir").error() == FsError::AlreadyExists);
            }
        }
    }
}
