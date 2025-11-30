#include <doctest/doctest.h>

import examples.filesystem.path_ops;
import examples.filesystem.traits;
import arc;

using namespace examples::filesystem;

TEST_CASE("PathOps")
{
    arc::test::Graph<PathOps> graph;
    auto pathOps = graph.node.asTrait(trait::pathOps);

    SUBCASE("normalise")
    {
        SUBCASE("empty path becomes root")
        {
            CHECK(pathOps.normalise("") == "/");
        }

        SUBCASE("root stays root")
        {
            CHECK(pathOps.normalise("/") == "/");
        }

        SUBCASE("adds leading slash")
        {
            CHECK(pathOps.normalise("foo") == "/foo");
            CHECK(pathOps.normalise("foo/bar") == "/foo/bar");
        }

        SUBCASE("removes trailing slashes")
        {
            CHECK(pathOps.normalise("/foo/") == "/foo");
            CHECK(pathOps.normalise("/foo/bar/") == "/foo/bar");
        }

        SUBCASE("collapses multiple slashes")
        {
            CHECK(pathOps.normalise("//foo") == "/foo");
            CHECK(pathOps.normalise("/foo//bar") == "/foo/bar");
            CHECK(pathOps.normalise("/foo///bar//") == "/foo/bar");
        }

        SUBCASE("resolves single dot")
        {
            CHECK(pathOps.normalise("/./foo") == "/foo");
            CHECK(pathOps.normalise("/foo/./bar") == "/foo/bar");
            CHECK(pathOps.normalise("/foo/.") == "/foo");
        }

        SUBCASE("resolves double dot")
        {
            CHECK(pathOps.normalise("/foo/bar/..") == "/foo");
            CHECK(pathOps.normalise("/foo/bar/../baz") == "/foo/baz");
            CHECK(pathOps.normalise("/foo/bar/baz/../..") == "/foo");
        }

        SUBCASE("double dot at root stays at root")
        {
            CHECK(pathOps.normalise("/..") == "/");
            CHECK(pathOps.normalise("/../foo") == "/foo");
        }
    }

    SUBCASE("parent")
    {
        CHECK(pathOps.parent("/") == "/");
        CHECK(pathOps.parent("/foo") == "/");
        CHECK(pathOps.parent("/foo/bar") == "/foo");
        CHECK(pathOps.parent("/foo/bar/baz") == "/foo/bar");
    }

    SUBCASE("filename")
    {
        CHECK(pathOps.filename("/") == "");
        CHECK(pathOps.filename("/foo") == "foo");
        CHECK(pathOps.filename("/foo/bar") == "bar");
        CHECK(pathOps.filename("/foo/bar.txt") == "bar.txt");
    }

    SUBCASE("join")
    {
        CHECK(pathOps.join("/", "foo") == "/foo");
        CHECK(pathOps.join("/foo", "bar") == "/foo/bar");
        CHECK(pathOps.join("/foo/", "bar") == "/foo/bar");
        // Absolute child path replaces base (Unix behavior)
        CHECK(pathOps.join("/foo", "/bar") == "/bar");
    }

    SUBCASE("isRoot")
    {
        CHECK(pathOps.isRoot("/") == true);
        CHECK(pathOps.isRoot("/foo") == false);
        // Empty string normalises to root
        CHECK(pathOps.isRoot("") == true);
    }
}
