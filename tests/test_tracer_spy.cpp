#include <doctest/doctest.h>

import arc.tests.test_tracer_spy;
import arc;
import std;

namespace arc::tests::test_tracer_spy {

/* arc-begin
export module arc.tests.test_tracer_spy;

namespace arc::tests::test_tracer_spy {

trait Adder
{
    add(int a, int b) -> int
}

trait Doubler
{
    twice(int x) -> int
}

trait Mirror
{
    bounce(int const* p) -> int
}

cluster Main [Root]
{
    adder = Root::Adder
    doubler = Root::Doubler
    mirror = Root::Mirror

    [trait::Adder]
    doubler --> adder
}

} // namespace arc::tests::test_tracer_spy
arc-end */

struct Root
{
    struct Adder : arc::NodeImpl<trait::Adder>
    {
        int add(int a, int b) const { return a + b; }
    };

    struct Doubler : arc::Node::
        Impl<trait::Doubler>::
        Uses<trait::Adder>
    {
        int twice(this auto& self, int x)
        {
            return self.getAdder().add(x, x);
        }
    };

    struct Mirror : arc::NodeImpl<trait::Mirror>
    {
        int bounce(int const* p) const { return p ? *p : -1; }
    };
};

SCENARIO(R"(TracerSpy emits a structured JSONL trace for agent debugging)")
{
    GIVEN(R"(a graph wired with TracerSpy as global)")
    {
        std::ostringstream trace;

        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        auto doubler = graph->doubler->asTrait(trait::doubler);

        WHEN(R"(a top-level call invokes a nested call)")
        {
            int const r = doubler.twice(21);

            graph.global->write(trace, arc::TracerSpy::Format::Jsonl);
            auto const out = trace.str();

            THEN(R"(result is correct and trace contains both enter/exit events)")
            {
                CHECK(r == 42);
                CHECK(out.find("\"evt\":\"enter\"") != std::string::npos);
                CHECK(out.find("\"evt\":\"exit\"")  != std::string::npos);
                CHECK(out.find("twice") != std::string::npos);
                CHECK(out.find("add")   != std::string::npos);
            }

            THEN(R"(call tree is recorded with parent/child relationship)")
            {
                // Doubler::twice (id 0) calls Adder::add (id 1, parent 0).
                CHECK(out.find("\"id\":0") != std::string::npos);
                CHECK(out.find("\"id\":1,\"parent\":0") != std::string::npos);
            }

            THEN(R"(per-method summary lists both methods with one call each)")
            {
                CHECK(out.find("\"evt\":\"summary\"") != std::string::npos);
                CHECK(out.find("\"calls\":1") != std::string::npos);
                CHECK(out.find("\"evt\":\"summary_meta\"") != std::string::npos);
                CHECK(out.find("\"max_depth\":2") != std::string::npos);
            }
        }
    }
}

SCENARIO(R"(TracerSpy emits a full JSONL + human trace for inspection)")
{
    GIVEN(R"(a graph with a multi-level call chain)")
    {
        std::ostringstream jsonlTrace;
        std::ostringstream humanTrace;

        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        WHEN(R"(twice(7) is invoked and trace is rendered in both formats)")
        {
            int const r1 = graph->doubler->asTrait(trait::doubler).twice(7);
            int const r2 = graph->doubler->asTrait(trait::doubler).twice(7);

            graph.global->write(jsonlTrace, arc::TracerSpy::Format::Jsonl);
            graph.global->write(humanTrace, arc::TracerSpy::Format::Human);

            THEN(R"(both produce identical results and a complete trace is emitted)")
            {
                CHECK(r1 == 14);
                CHECK(r2 == 14);

                MESSAGE("\n--- TracerSpy JSONL trace ---\n" << jsonlTrace.str());
                MESSAGE("\n--- TracerSpy human trace ---\n" << humanTrace.str());

                // Sanity: every enter has a matching exit (or throw).
                auto const j = jsonlTrace.str();
                auto count = [&](std::string_view needle) {
                    std::size_t n = 0;
                    for (std::size_t pos = 0;
                         (pos = j.find(needle, pos)) != std::string::npos;
                         pos += needle.size())
                        ++n;
                    return n;
                };
                auto const enters = count("\"evt\":\"enter\"");
                auto const exits  = count("\"evt\":\"exit\"");
                auto const throws = count("\"evt\":\"throw\"");
                CHECK(enters == exits + throws);
                CHECK(enters >= 2);
            }
        }
    }
}

SCENARIO(R"(TracerSpy records pointer arguments as address and pointee value)")
{
    GIVEN(R"(a Mirror node whose trait takes int const*)")
    {
        std::ostringstream trace;

        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        int answer = 42;
        int const* p = &answer;
        int const* np = nullptr;

        WHEN(R"(bounce is called with a valid pointer and then a null pointer)")
        {
            int const r1 = graph->mirror->asTrait(trait::mirror).bounce(p);
            int const r2 = graph->mirror->asTrait(trait::mirror).bounce(np);

            graph.global->write(trace, arc::TracerSpy::Format::Human);
            auto const out = trace.str();

            MESSAGE("\n--- TracerSpy pointer trace ---\n" << out);

            THEN(R"(non-null pointer renders as 0xADDR -> value, null renders as nullptr)")
            {
                CHECK(r1 == 42);
                CHECK(r2 == -1);
                CHECK(out.find(" -> 42") != std::string::npos);
                CHECK(out.find("nullptr") != std::string::npos);
                CHECK(out.find("0x") != std::string::npos);
                // Node-handle identity surfaced through the spy caller.
                CHECK(out.find("[arc::tests::test_tracer_spy::Root::Mirror]") != std::string::npos);
            }
        }
    }
}

SCENARIO(R"(TracerSpy captures exceptions thrown across the trait boundary)")
{
    struct ThrowingAdder : arc::NodeImpl<trait::Adder>
    {
        int impl(trait::Adder::add, int, int) const
        {
            throw std::runtime_error("boom");
        }
    };

    struct ThrowRoot
    {
        using Adder   = ThrowingAdder;
        using Doubler = Root::Doubler;
        using Mirror  = Root::Mirror;
    };

    GIVEN(R"(an adder that throws)")
    {
        std::ostringstream trace;

        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, ThrowRoot> graph{
            .global{},
            .main{}
        };

        WHEN(R"(the doubler invokes the throwing adder)")
        {
            CHECK_THROWS_AS(
                graph->doubler->asTrait(trait::doubler).twice(1),
                std::runtime_error);

            graph.global->write(trace, arc::TracerSpy::Format::Jsonl);
            auto const out = trace.str();

            THEN(R"(trace records a throw event with the exception message)")
            {
                CHECK(out.find("\"evt\":\"throw\"") != std::string::npos);
                CHECK(out.find("boom") != std::string::npos);
                CHECK(out.find("\"exc\":1") != std::string::npos);
            }
        }
    }
}

} // namespace arc::tests::test_tracer_spy
