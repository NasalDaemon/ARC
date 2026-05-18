import examples.filesystem.path_ops;
import examples.filesystem.traits;
import arc;

#include "arc/doctest.h"

using namespace examples::filesystem;

SCENARIO(R"(PathOps normalisation and navigation)")
{
    arc::test::Graph<node::PathOps> graph;
    auto pathOps = graph.node.asTrait(trait::pathOps);

    GIVEN(R"(a PathOps trait)")
    {
        WHEN(R"(normalising paths)")
        {
            THEN(R"(root stays root)")
            {
                CHECK(pathOps.normalise("/") == "/");
            }

            THEN(R"(leading slash is added)")
            {
                CHECK(pathOps.normalise("foo") == "/foo");
                CHECK(pathOps.normalise("foo/bar") == "/foo/bar");
            }

            THEN(R"(trailing slashes are removed)")
            {
                CHECK(pathOps.normalise("/foo/") == "/foo");
                CHECK(pathOps.normalise("/foo/bar/") == "/foo/bar");
            }

            THEN(R"(multiple slashes are collapsed)")
            {
                CHECK(pathOps.normalise("//foo") == "/foo");
                CHECK(pathOps.normalise("/foo//bar") == "/foo/bar");
                CHECK(pathOps.normalise("/foo///bar//") == "/foo/bar");
            }

            THEN(R"(single dot is resolved)")
            {
                CHECK(pathOps.normalise("/./foo") == "/foo");
                CHECK(pathOps.normalise("/foo/./bar") == "/foo/bar");
                CHECK(pathOps.normalise("/foo/.") == "/foo");
            }

            THEN(R"(double dot is resolved)")
            {
                CHECK(pathOps.normalise("/foo/bar/..") == "/foo");
                CHECK(pathOps.normalise("/foo/bar/../baz") == "/foo/baz");
                CHECK(pathOps.normalise("/foo/bar/baz/../..") == "/foo");
            }

            THEN(R"(double dot at root stays at root)")
            {
                CHECK(pathOps.normalise("/..") == "/");
                CHECK(pathOps.normalise("/../foo") == "/foo");
            }
        }

        AND_WHEN(R"(empty path is passed to normalise)")
        {
            THEN(R"(contract violation is thrown)")
            {
                CHECK_THROWS_AS(pathOps.normalise(""), arc::ContractViolation);
            }
        }

        WHEN(R"(getting the parent path)")
        {
            THEN(R"(returns correct parents)")
            {
                CHECK(pathOps.parent("/") == "/");
                CHECK(pathOps.parent("/foo") == "/");
                CHECK(pathOps.parent("/foo/bar") == "/foo");
                CHECK(pathOps.parent("/foo/bar/baz") == "/foo/bar");
            }
        }

        AND_WHEN(R"(empty path is passed to parent)")
        {
            THEN(R"(contract violation is thrown)")
            {
                CHECK_THROWS_AS(pathOps.parent(""), arc::ContractViolation);
            }
        }

        WHEN(R"(getting the filename)")
        {
            THEN(R"(returns correct filenames)")
            {
                CHECK(pathOps.filename("/") == "");
                CHECK(pathOps.filename("/foo") == "foo");
                CHECK(pathOps.filename("/foo/bar") == "bar");
                CHECK(pathOps.filename("/foo/bar.txt") == "bar.txt");
            }
        }

        AND_WHEN(R"(empty path is passed to filename)")
        {
            THEN(R"(contract violation is thrown)")
            {
                CHECK_THROWS_AS(pathOps.filename(""), arc::ContractViolation);
            }
        }

        WHEN(R"(joining paths)")
        {
            THEN(R"(joins correctly)")
            {
                CHECK(pathOps.join("/", "foo") == "/foo");
                CHECK(pathOps.join("/foo", "bar") == "/foo/bar");
                CHECK(pathOps.join("/foo/", "bar") == "/foo/bar");
                CHECK(pathOps.join("/foo", "/bar") == "/bar");
            }
        }

        AND_WHEN(R"(empty path is passed to join)")
        {
            THEN(R"(contract violation is thrown)")
            {
                CHECK_THROWS_AS(pathOps.join("", "foo"), arc::ContractViolation);
            }
        }

        WHEN(R"(checking if a path is root)")
        {
            THEN(R"(identifies root correctly)")
            {
                CHECK(pathOps.isRoot("/") == true);
                CHECK(pathOps.isRoot("/foo") == false);
            }
        }

        AND_WHEN(R"(empty path is passed to isRoot)")
        {
            THEN(R"(contract violation is thrown)")
            {
                CHECK_THROWS_AS(pathOps.isRoot(""), arc::ContractViolation);
            }
        }
    }
}
