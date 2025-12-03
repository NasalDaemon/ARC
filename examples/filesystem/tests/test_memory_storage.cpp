#include <doctest/doctest.h>

import examples.filesystem.memory_storage;
import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;

using namespace examples::filesystem;

TEST_CASE("MemoryStorage")
{
    arc::test::Graph<MemoryStorage> graph;
    auto storage = graph.node.asTrait(trait::storage);

    SUBCASE("initially contains only root")
    {
        auto root = storage.get("/");
        REQUIRE(root != nullptr);
        CHECK(root->isDir() == true);

        auto nonexistent = storage.get("/anything");
        CHECK(nonexistent == nullptr);
    }

    SUBCASE("put and get file")
    {
        REQUIRE(storage.put("/test.txt", InMemoryEntry::file("hello")).has_value());

        auto entry = storage.get("/test.txt");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir() == false);
        CHECK(entry->content() == "hello");
    }

    SUBCASE("put and get directory")
    {
        REQUIRE(storage.put("/mydir", InMemoryEntry::directory()).has_value());

        auto entry = storage.get("/mydir");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir() == true);
    }

    SUBCASE("put overwrites existing")
    {
        REQUIRE(storage.children("/").size() == 0);
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("first")).has_value());
        CHECK(storage.children("/").size() == 1);
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("second")).has_value());
        CHECK(storage.children("/").size() == 1); // root has only "file.txt"

        auto entry = storage.get("/file.txt");
        REQUIRE(entry != nullptr);
        CHECK(entry->content() == "second");
    }

    SUBCASE("erase removes entry")
    {
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("data")).has_value());
        REQUIRE(storage.get("/file.txt") != nullptr);
        REQUIRE(storage.children("/").size() == 1);

        bool erased = storage.erase("/file.txt");
        CHECK(erased == true);
        CHECK(storage.get("/file.txt") == nullptr);
        CHECK(storage.children("/").size() == 0);
    }

    SUBCASE("erase nonexistent returns false")
    {
        bool erased = storage.erase("/nonexistent");
        CHECK(erased == false);
    }

    SUBCASE("children lists entries with given prefix")
    {
        REQUIRE(storage.put("/docs", InMemoryEntry::directory()).has_value());
        REQUIRE(storage.put("/docs/readme.md", InMemoryEntry::file("# Readme")).has_value());
        REQUIRE(storage.put("/docs/guide.md", InMemoryEntry::file("# Guide")).has_value());
        REQUIRE(storage.put("/src", InMemoryEntry::directory()).has_value());
        REQUIRE(storage.put("/src/main.cpp", InMemoryEntry::file("int main() {}")).has_value());

        auto docsChildren = storage.children("/docs");
        CHECK(docsChildren.size() == 2);
        CHECK(docsChildren[0] == "guide.md");
        CHECK(docsChildren[1] == "readme.md");

        auto rootChildren = storage.children("/");
        CHECK(rootChildren.size() == 2);
        CHECK(rootChildren[0] == "docs");
        CHECK(rootChildren[1] == "src");
    }

    SUBCASE("children returns empty for nonexistent path")
    {
        auto children = storage.children("/nonexistent");
        CHECK(children.empty());
    }

    SUBCASE("children excludes nested entries")
    {
        REQUIRE(storage.put("/a", InMemoryEntry::directory()).has_value());
        REQUIRE(storage.put("/a/b", InMemoryEntry::directory()).has_value());
        REQUIRE(storage.put("/a/b/c", InMemoryEntry::file("deep")).has_value());

        auto aChildren = storage.children("/a");
        CHECK(aChildren.size() == 1);
        CHECK(aChildren[0] == "b");
    }

    SUBCASE("put returns error when overwriting directory with file")
    {
        REQUIRE(storage.put("/mydir", InMemoryEntry::directory()).has_value());

        auto result = storage.put("/mydir", InMemoryEntry::file("content"));
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == FsError::IsADirectory);

        // Directory should still exist unchanged
        auto entry = storage.get("/mydir");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir());
    }

    SUBCASE("put allows overwriting directory with directory")
    {
        REQUIRE(storage.put("/mydir", InMemoryEntry::directory()).has_value());

        auto result = storage.put("/mydir", InMemoryEntry::directory());
        REQUIRE(result.has_value());

        auto entry = storage.get("/mydir");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir());
    }

    SUBCASE("put allows overwriting file with file")
    {
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("first")).has_value());

        auto result = storage.put("/file.txt", InMemoryEntry::file("second"));
        REQUIRE(result.has_value());

        auto entry = storage.get("/file.txt");
        REQUIRE(entry != nullptr);
        CHECK_FALSE(entry->isDir());
        CHECK(entry->content() == "second");
    }
}
