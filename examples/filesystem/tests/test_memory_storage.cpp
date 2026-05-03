#include <doctest/doctest.h>

import examples.filesystem.memory_storage;
import examples.filesystem.types;
import examples.filesystem.traits;
import arc;

using namespace examples::filesystem;

SCENARIO(R"(MemoryStorage stores and retrieves filesystem entries)")
{
    arc::test::Graph<node::MemoryStorage> graph;
    auto storage = graph.node.asTrait(trait::storage);

    GIVEN(R"(a fresh storage with only the root directory)")
    {
        WHEN(R"(retrieving the root)")
        {
            auto root = storage.get("/");

            THEN(R"(root exists and is a directory)")
            {
                REQUIRE(root != nullptr);
                CHECK(root->isDir() == true);
            }
        }

        WHEN(R"(retrieving a nonexistent path)")
        {
            auto nonexistent = storage.get("/anything");

            THEN(R"(it returns nullptr)")
            {
                CHECK(nonexistent == nullptr);
            }
        }

        WHEN(R"(listing children of a nonexistent path)")
        {
            auto children = storage.children("/nonexistent");

            THEN(R"(the result is empty)")
            {
                CHECK(children.empty());
            }
        }

        WHEN(R"(erasing a nonexistent entry)")
        {
            bool erased = storage.erase("/nonexistent");

            THEN(R"(erase returns false)")
            {
                CHECK(erased == false);
            }
        }
    }

    GIVEN(R"(storage after putting a file)")
    {
        REQUIRE(storage.put("/test.txt", Entry::file("hello")).has_value());

        WHEN(R"(retrieving the file)")
        {
            auto entry = storage.get("/test.txt");

            THEN(R"(the file exists with the correct content)")
            {
                REQUIRE(entry != nullptr);
                CHECK(entry->isDir() == false);
                CHECK(entry->content() == "hello");
            }
        }
    }

    GIVEN(R"(storage after putting a directory)")
    {
        REQUIRE(storage.put("/mydir", Entry::directory()).has_value());

        WHEN(R"(retrieving the directory)")
        {
            auto entry = storage.get("/mydir");

            THEN(R"(the directory exists)")
            {
                REQUIRE(entry != nullptr);
                CHECK(entry->isDir() == true);
            }
        }
    }

    GIVEN(R"(storage with an existing file)")
    {
        REQUIRE(storage.children("/").size() == 0);
        REQUIRE(storage.put("/file.txt", Entry::file("first")).has_value());
        CHECK(storage.children("/").size() == 1);

        WHEN(R"(overwriting the file with new content)")
        {
            REQUIRE(storage.put("/file.txt", Entry::file("second")).has_value());

            THEN(R"(the root still has only one entry)")
            {
                CHECK(storage.children("/").size() == 1);
            }

            THEN(R"(the file contains the new content)")
            {
                auto entry = storage.get("/file.txt");
                REQUIRE(entry != nullptr);
                CHECK(entry->content() == "second");
            }
        }
    }

    GIVEN(R"(storage with an existing file that will be erased)")
    {
        REQUIRE(storage.put("/file.txt", Entry::file("data")).has_value());
        REQUIRE(storage.get("/file.txt") != nullptr);
        REQUIRE(storage.children("/").size() == 1);

        WHEN(R"(erasing the file)")
        {
            bool erased = storage.erase("/file.txt");

            THEN(R"(erase succeeds)")
            {
                CHECK(erased == true);
            }

            THEN(R"(the file is gone)")
            {
                CHECK(storage.get("/file.txt") == nullptr);
            }

            THEN(R"(the root has no children)")
            {
                CHECK(storage.children("/").size() == 0);
            }
        }
    }

    GIVEN(R"(storage with docs and src directories containing files)")
    {
        REQUIRE(storage.put("/docs", Entry::directory()).has_value());
        REQUIRE(storage.put("/docs/readme.md", Entry::file("# Readme")).has_value());
        REQUIRE(storage.put("/docs/guide.md", Entry::file("# Guide")).has_value());
        REQUIRE(storage.put("/src", Entry::directory()).has_value());
        REQUIRE(storage.put("/src/main.cpp", Entry::file("int main() {}")).has_value());

        WHEN(R"(listing children of /docs)")
        {
            auto docsChildren = storage.children("/docs");

            THEN(R"(it returns the two markdown files in sorted order)")
            {
                CHECK(docsChildren.size() == 2);
                CHECK(docsChildren[0] == "guide.md");
                CHECK(docsChildren[1] == "readme.md");
            }
        }

        WHEN(R"(listing children of root)")
        {
            auto rootChildren = storage.children("/");

            THEN(R"(it returns the two top-level directories in sorted order)")
            {
                CHECK(rootChildren.size() == 2);
                CHECK(rootChildren[0] == "docs");
                CHECK(rootChildren[1] == "src");
            }
        }
    }

    GIVEN(R"(storage with a nested directory structure)")
    {
        REQUIRE(storage.put("/a", Entry::directory()).has_value());
        REQUIRE(storage.put("/a/b", Entry::directory()).has_value());
        REQUIRE(storage.put("/a/b/c", Entry::file("deep")).has_value());

        WHEN(R"(listing children of /a)")
        {
            auto aChildren = storage.children("/a");

            THEN(R"(only direct children are listed)")
            {
                CHECK(aChildren.size() == 1);
                CHECK(aChildren[0] == "b");
            }
        }
    }

    GIVEN(R"(storage with an existing directory)")
    {
        REQUIRE(storage.put("/mydir", Entry::directory()).has_value());

        WHEN(R"(overwriting the directory with a file)")
        {
            auto result = storage.put("/mydir", Entry::file("content"));

            THEN(R"(put returns IsADirectory error)")
            {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == FsError::IsADirectory);
            }

            THEN(R"(the directory still exists unchanged)")
            {
                auto entry = storage.get("/mydir");
                REQUIRE(entry != nullptr);
                CHECK(entry->isDir());
            }
        }

        WHEN(R"(overwriting the directory with another directory)")
        {
            auto result = storage.put("/mydir", Entry::directory());

            THEN(R"(put succeeds)")
            {
                REQUIRE(result.has_value());
            }

            THEN(R"(the directory still exists)")
            {
                auto entry = storage.get("/mydir");
                REQUIRE(entry != nullptr);
                CHECK(entry->isDir());
            }
        }
    }

    GIVEN(R"(storage with an existing file)")
    {
        REQUIRE(storage.put("/file.txt", Entry::file("first")).has_value());

        WHEN(R"(overwriting the file with another file)")
        {
            auto result = storage.put("/file.txt", Entry::file("second"));

            THEN(R"(put succeeds)")
            {
                REQUIRE(result.has_value());
            }

            THEN(R"(the file has updated content)")
            {
                auto entry = storage.get("/file.txt");
                REQUIRE(entry != nullptr);
                CHECK_FALSE(entry->isDir());
                CHECK(entry->content() == "second");
            }
        }
    }
}
