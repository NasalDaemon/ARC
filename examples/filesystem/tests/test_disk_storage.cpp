#include <doctest/doctest.h>

import examples.filesystem.disk_storage;
import examples.filesystem.entry;
import examples.filesystem.traits;
import arc;
import std;

using namespace examples::filesystem;

namespace {

// RAII helper to create and cleanup a temporary test directory
struct TempDir
{
    std::filesystem::path path;

    TempDir()
    {
        auto tmp = std::filesystem::temp_directory_path();
        path = tmp / ("arc_disk_storage_test_" + std::to_string(static_cast<unsigned int>(std::random_device{}())));
        std::filesystem::create_directories(path);
    }

    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    TempDir(TempDir const&) = delete;
    auto operator=(TempDir const&) -> TempDir& = delete;
};

} // namespace

TEST_CASE("DiskStorage")
{
    TempDir tempDir;
    arc::test::Graph<DiskStorage> graph;
    auto dirSync = graph.node.asTrait(trait::directorySync);
    REQUIRE(dirSync.loadFromDirectory(tempDir.path.string()).has_value());
    auto storage = graph.node.asTrait(trait::storage);

    SUBCASE("initially contains only root")
    {
        auto root = storage.get("/");
        REQUIRE(root.has_value());
        CHECK(root->isDir() == true);

        auto nonexistent = storage.get("/anything");
        CHECK(!nonexistent.has_value());
    }

    SUBCASE("put and get file")
    {
        REQUIRE(storage.put("/test.txt", InMemoryEntry::file("hello")).has_value());

        auto entry = storage.get("/test.txt");
        REQUIRE(entry.has_value());
        CHECK(entry->isDir() == false);
        CHECK(entry->content() == "hello");

        // Verify it's actually on disk
        auto diskPath = tempDir.path / "test.txt";
        REQUIRE(std::filesystem::exists(diskPath));
        std::ifstream file{diskPath};
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(content == "hello");
    }

    SUBCASE("put and get directory")
    {
        REQUIRE(storage.put("/mydir", InMemoryEntry::directory()).has_value());

        auto entry = storage.get("/mydir");
        REQUIRE(entry.has_value());
        CHECK(entry->isDir() == true);

        // Verify it's actually on disk
        auto diskPath = tempDir.path / "mydir";
        CHECK(std::filesystem::is_directory(diskPath));
    }

    SUBCASE("put overwrites existing file")
    {
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("first")).has_value());
        REQUIRE(storage.put("/file.txt", InMemoryEntry::file("second")).has_value());

        auto entry = storage.get("/file.txt");
        REQUIRE(entry.has_value());
        CHECK(entry->content() == "second");
    }

    SUBCASE("put creates parent directories")
    {
        REQUIRE(storage.put("/deep/nested/file.txt", InMemoryEntry::file("content")).has_value());

        auto entry = storage.get("/deep/nested/file.txt");
        REQUIRE(entry.has_value());
        CHECK(entry->content() == "content");

        CHECK(std::filesystem::is_directory(tempDir.path / "deep"));
        CHECK(std::filesystem::is_directory(tempDir.path / "deep" / "nested"));
    }

    SUBCASE("erase removes file")
    {
        storage.put("/file.txt", InMemoryEntry::file("data"));
        REQUIRE(storage.get("/file.txt").has_value());

        bool erased = storage.erase("/file.txt");
        CHECK(erased == true);
        CHECK(!storage.get("/file.txt").has_value());
        CHECK(!std::filesystem::exists(tempDir.path / "file.txt"));
    }

    SUBCASE("erase removes empty directory")
    {
        storage.put("/emptydir", InMemoryEntry::directory());
        REQUIRE(storage.get("/emptydir").has_value());

        bool erased = storage.erase("/emptydir");
        CHECK(erased == true);
        CHECK(!storage.get("/emptydir").has_value());
    }

    SUBCASE("erase nonexistent returns false")
    {
        bool erased = storage.erase("/nonexistent");
        CHECK(erased == false);
    }

    SUBCASE("children lists directory contents")
    {
        storage.put("/docs", InMemoryEntry::directory());
        storage.put("/docs/readme.md", InMemoryEntry::file("# Readme"));
        storage.put("/docs/guide.md", InMemoryEntry::file("# Guide"));
        storage.put("/src", InMemoryEntry::directory());
        storage.put("/src/main.cpp", InMemoryEntry::file("int main() {}"));

        auto docsChildren = storage.children("/docs");
        CHECK(docsChildren.size() == 2);
        // Note: filesystem order may vary, so we check both are present
        bool hasReadme = std::ranges::find(docsChildren, "readme.md") != docsChildren.end();
        bool hasGuide = std::ranges::find(docsChildren, "guide.md") != docsChildren.end();
        CHECK(hasReadme);
        CHECK(hasGuide);

        auto rootChildren = storage.children("/");
        CHECK(rootChildren.size() == 2);
        bool hasDocs = std::ranges::find(rootChildren, "docs") != rootChildren.end();
        bool hasSrc = std::ranges::find(rootChildren, "src") != rootChildren.end();
        CHECK(hasDocs);
        CHECK(hasSrc);
    }

    SUBCASE("children returns empty for nonexistent path")
    {
        auto children = storage.children("/nonexistent");
        CHECK(children.empty());
    }

    SUBCASE("children excludes nested entries")
    {
        storage.put("/a", InMemoryEntry::directory());
        storage.put("/a/b", InMemoryEntry::directory());
        storage.put("/a/b/c", InMemoryEntry::file("deep"));

        auto aChildren = storage.children("/a");
        CHECK(aChildren.size() == 1);
        CHECK(aChildren[0] == "b");
    }
}

TEST_CASE("DiskStorage reads pre-existing files")
{
    TempDir tempDir;

    // Create files directly on disk before loading
    std::filesystem::create_directories(tempDir.path / "existing");
    std::ofstream{tempDir.path / "existing" / "data.txt"} << "pre-existing content";

    arc::test::Graph<DiskStorage> graph;
    auto dirSync = graph.node.asTrait(trait::directorySync);
    REQUIRE(dirSync.loadFromDirectory(tempDir.path.string()).has_value());
    auto storage = graph.node.asTrait(trait::storage);

    SUBCASE("can read pre-existing directory")
    {
        auto entry = storage.get("/existing");
        REQUIRE(entry.has_value());
        CHECK(entry->isDir() == true);
    }

    SUBCASE("can read pre-existing file")
    {
        auto entry = storage.get("/existing/data.txt");
        REQUIRE(entry.has_value());
        CHECK(entry->isDir() == false);
        CHECK(entry->content() == "pre-existing content");
    }

    SUBCASE("can list pre-existing children")
    {
        auto children = storage.children("/existing");
        REQUIRE(children.size() == 1);
        CHECK(children[0] == "data.txt");
    }
}
