import examples.filesystem.disk_storage;
import examples.filesystem.types;
import examples.filesystem.traits;
import arc;
import std;

#include "arc/doctest.h"

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

SCENARIO(R"(DiskStorage stores and retrieves entries on disk)")
{
    TempDir tempDir;
    arc::test::Graph<node::DiskStorage> graph;
    auto dirSync = graph.node.asTrait(trait::directorySync);
    REQUIRE(dirSync.loadFromDirectory(tempDir.path.string()).has_value());
    auto storage = graph.node.asTrait(trait::storage);

    GIVEN(R"(a freshly loaded empty storage)")
    {
        WHEN(R"(querying the root path)")
        {
            auto root = storage.get("/");

            THEN(R"(the root exists and is a directory)")
            {
                REQUIRE(root.has_value());
                CHECK(root->isDir() == true);
            }
        }

        WHEN(R"(querying a nonexistent path)")
        {
            auto nonexistent = storage.get("/anything");

            THEN(R"(no entry is returned)")
            {
                CHECK(!nonexistent.has_value());
            }
        }
    }

    GIVEN(R"(a file is put into storage)")
    {
        REQUIRE(storage.put("/test.txt", Entry::file("hello")).has_value());

        WHEN(R"(getting the file back)")
        {
            auto entry = storage.get("/test.txt");

            THEN(R"(the entry is a file with the correct content)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->isDir() == false);
                CHECK(entry->content() == "hello");
            }
        }

        WHEN(R"(checking the file on disk)")
        {
            auto diskPath = tempDir.path / "test.txt";

            THEN(R"(the file exists with the correct content)")
            {
                REQUIRE(std::filesystem::exists(diskPath));
                std::ifstream file{diskPath};
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                CHECK(content == "hello");
            }
        }
    }

    GIVEN(R"(a directory is put into storage)")
    {
        REQUIRE(storage.put("/mydir", Entry::directory()).has_value());

        WHEN(R"(getting the directory back)")
        {
            auto entry = storage.get("/mydir");

            THEN(R"(the entry is a directory)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->isDir() == true);
            }
        }

        WHEN(R"(checking the directory on disk)")
        {
            auto diskPath = tempDir.path / "mydir";

            THEN(R"(a directory exists on disk)")
            {
                CHECK(std::filesystem::is_directory(diskPath));
            }
        }
    }

    GIVEN(R"(a file is put and then overwritten)")
    {
        REQUIRE(storage.put("/file.txt", Entry::file("first")).has_value());
        REQUIRE(storage.put("/file.txt", Entry::file("second")).has_value());

        WHEN(R"(getting the overwritten file)")
        {
            auto entry = storage.get("/file.txt");

            THEN(R"(the entry has the new content)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->content() == "second");
            }
        }
    }

    GIVEN(R"(a file is put into a nested path)")
    {
        REQUIRE(storage.put("/deep/nested/file.txt", Entry::file("content")).has_value());

        WHEN(R"(getting the nested file)")
        {
            auto entry = storage.get("/deep/nested/file.txt");

            THEN(R"(the entry is returned with correct content)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->content() == "content");
            }
        }

        WHEN(R"(checking the parent directories on disk)")
        {
            THEN(R"(all parent directories are created)")
            {
                CHECK(std::filesystem::is_directory(tempDir.path / "deep"));
                CHECK(std::filesystem::is_directory(tempDir.path / "deep" / "nested"));
            }
        }
    }

    GIVEN(R"(a file exists in storage)")
    {
        storage.put("/file.txt", Entry::file("data"));
        REQUIRE(storage.get("/file.txt").has_value());

        WHEN(R"(erasing the file)")
        {
            bool erased = storage.erase("/file.txt");

            THEN(R"(the erase succeeds and the file is gone)")
            {
                CHECK(erased == true);
                CHECK(!storage.get("/file.txt").has_value());
                CHECK(!std::filesystem::exists(tempDir.path / "file.txt"));
            }
        }
    }

    GIVEN(R"(an empty directory exists in storage)")
    {
        storage.put("/emptydir", Entry::directory());
        REQUIRE(storage.get("/emptydir").has_value());

        WHEN(R"(erasing the directory)")
        {
            bool erased = storage.erase("/emptydir");

            THEN(R"(the erase succeeds and the directory is gone)")
            {
                CHECK(erased == true);
                CHECK(!storage.get("/emptydir").has_value());
            }
        }
    }

    GIVEN(R"(no entry exists at a path)")
    {
        WHEN(R"(erasing the nonexistent path)")
        {
            bool erased = storage.erase("/nonexistent");

            THEN(R"(erase returns false)")
            {
                CHECK(erased == false);
            }
        }
    }

    GIVEN(R"(multiple files and directories are put into storage)")
    {
        storage.put("/docs", Entry::directory());
        storage.put("/docs/readme.md", Entry::file("# Readme"));
        storage.put("/docs/guide.md", Entry::file("# Guide"));
        storage.put("/src", Entry::directory());
        storage.put("/src/main.cpp", Entry::file("int main() {}"));

        WHEN(R"(listing children of a subdirectory)")
        {
            auto docsChildren = storage.children("/docs");

            THEN(R"(only direct children are listed)")
            {
                CHECK(docsChildren.size() == 2);
                bool hasReadme = std::ranges::find(docsChildren, "readme.md") != docsChildren.end();
                bool hasGuide = std::ranges::find(docsChildren, "guide.md") != docsChildren.end();
                CHECK(hasReadme);
                CHECK(hasGuide);
            }
        }

        WHEN(R"(listing children of the root)")
        {
            auto rootChildren = storage.children("/");

            THEN(R"(top-level entries are listed)")
            {
                CHECK(rootChildren.size() == 2);
                bool hasDocs = std::ranges::find(rootChildren, "docs") != rootChildren.end();
                bool hasSrc = std::ranges::find(rootChildren, "src") != rootChildren.end();
                CHECK(hasDocs);
                CHECK(hasSrc);
            }
        }
    }

    GIVEN(R"(no entry exists at a path)")
    {
        WHEN(R"(listing children of the nonexistent path)")
        {
            auto children = storage.children("/nonexistent");

            THEN(R"(an empty list is returned)")
            {
                CHECK(children.empty());
            }
        }
    }

    GIVEN(R"(a deeply nested directory structure)")
    {
        storage.put("/a", Entry::directory());
        storage.put("/a/b", Entry::directory());
        storage.put("/a/b/c", Entry::file("deep"));

        WHEN(R"(listing children of /a)")
        {
            auto aChildren = storage.children("/a");

            THEN(R"(only direct children are returned, not nested entries)")
            {
                CHECK(aChildren.size() == 1);
                CHECK(aChildren[0] == "b");
            }
        }
    }
}

SCENARIO(R"(DiskStorage reads pre-existing files from disk)")
{
    TempDir tempDir;

    // Create files directly on disk before loading
    std::filesystem::create_directories(tempDir.path / "existing");
    std::ofstream{tempDir.path / "existing" / "data.txt"} << "pre-existing content";

    arc::test::Graph<node::DiskStorage> graph;
    auto dirSync = graph.node.asTrait(trait::directorySync);
    REQUIRE(dirSync.loadFromDirectory(tempDir.path.string()).has_value());
    auto storage = graph.node.asTrait(trait::storage);

    GIVEN(R"(a directory that existed before loading)")
    {
        WHEN(R"(getting the pre-existing directory)")
        {
            auto entry = storage.get("/existing");

            THEN(R"(the directory is found)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->isDir() == true);
            }
        }

        WHEN(R"(getting a pre-existing file)")
        {
            auto entry = storage.get("/existing/data.txt");

            THEN(R"(the file is found with the correct content)")
            {
                REQUIRE(entry.has_value());
                CHECK(entry->isDir() == false);
                CHECK(entry->content() == "pre-existing content");
            }
        }

        WHEN(R"(listing children of the pre-existing directory)")
        {
            auto children = storage.children("/existing");

            THEN(R"(the pre-existing file is listed)")
            {
                REQUIRE(children.size() == 1);
                CHECK(children[0] == "data.txt");
            }
        }
    }
}
