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
        CHECK(root->isDir == true);

        auto nonexistent = storage.get("/anything");
        CHECK(nonexistent == nullptr);
    }

    SUBCASE("put and get file")
    {
        storage.put("/test.txt", Entry::file("hello"));

        auto entry = storage.get("/test.txt");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir == false);
        CHECK(entry->content == "hello");
    }

    SUBCASE("put and get directory")
    {
        storage.put("/mydir", Entry::directory());

        auto entry = storage.get("/mydir");
        REQUIRE(entry != nullptr);
        CHECK(entry->isDir == true);
    }

    SUBCASE("put overwrites existing")
    {
        storage.put("/file.txt", Entry::file("first"));
        storage.put("/file.txt", Entry::file("second"));

        auto entry = storage.get("/file.txt");
        REQUIRE(entry != nullptr);
        CHECK(entry->content == "second");
    }

    SUBCASE("erase removes entry")
    {
        storage.put("/file.txt", Entry::file("data"));
        REQUIRE(storage.get("/file.txt") != nullptr);

        bool erased = storage.erase("/file.txt");
        CHECK(erased == true);
        CHECK(storage.get("/file.txt") == nullptr);
    }

    SUBCASE("erase nonexistent returns false")
    {
        bool erased = storage.erase("/nonexistent");
        CHECK(erased == false);
    }

    SUBCASE("children lists entries with given prefix")
    {
        storage.put("/docs", Entry::directory());
        storage.put("/docs/readme.md", Entry::file("# Readme"));
        storage.put("/docs/guide.md", Entry::file("# Guide"));
        storage.put("/src", Entry::directory());
        storage.put("/src/main.cpp", Entry::file("int main() {}"));

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
        storage.put("/a", Entry::directory());
        storage.put("/a/b", Entry::directory());
        storage.put("/a/b/c", Entry::file("deep"));

        auto aChildren = storage.children("/a");
        CHECK(aChildren.size() == 1);
        CHECK(aChildren[0] == "b");
    }
}
