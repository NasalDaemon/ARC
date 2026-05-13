#include <doctest/doctest.h>

#if !ARC_IMPORT_STD
#include <exception>
#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#endif

import arc.tests.test_tracer_spy;
import arc;

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

SCENARIO(R"(TracerSpy ring buffer evicts oldest events while per-method stats survive)")
{
    GIVEN(R"(a TracerSpy sized to 4 events)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{{.maxEvents = 4}},
            .main{}
        };

        WHEN(R"(many calls overflow the ring)")
        {
            // Each twice() emits 2 enter + 2 exit events (Doubler::twice + Adder::add).
            constexpr int N = 10;
            for (int i = 0; i < N; ++i)
                graph->doubler->asTrait(trait::doubler).twice(i);

            std::ostringstream trace;
            graph.global->write(trace, arc::TracerSpy::Format::Jsonl);
            auto const out = trace.str();

            THEN(R"(events buffer is bounded to maxEvents)")
            {
                CHECK(graph.global->getEvents().size() == 4);
            }

            THEN(R"(per-method stats accumulate across evictions)")
            {
                std::size_t twiceCalls = 0;
                std::size_t addCalls = 0;
                for (auto const& [method, stat] : graph.global->getStats())
                {
                    if (method.find("twice") != std::string_view::npos)
                        twiceCalls = stat.calls;
                    else if (method.find("add") != std::string_view::npos)
                        addCalls = stat.calls;
                }
                CHECK(twiceCalls == N);
                CHECK(addCalls == N);
                CHECK(graph.global->getTotalCalls() == 2 * N);
            }

            THEN(R"(summary_meta reports emitted/in_buffer/evicted counts)")
            {
                CHECK(out.find("\"events_in_buffer\":4") != std::string::npos);
                CHECK(out.find("\"events_emitted\":40") != std::string::npos);
                CHECK(out.find("\"events_evicted\":36") != std::string::npos);
            }

            THEN(R"(buffer reports correct begin/end ids after eviction)")
            {
                auto const& events = graph.global->getEvents();
                // 40 events were appended; the ring holds the last 4.
                CHECK(events.end_id() == 40);
                CHECK(events.begin_id() == 36);
                CHECK(events.size() == 4);
            }

            THEN(R"(only events from the final twice() invocation are retained)")
            {
                auto const& events = graph.global->getEvents();
                // Last twice() corresponds to call ids 18 (twice) and 19 (add).
                std::size_t const lastTwice = graph.global->getTotalCalls() - 2;
                std::size_t const lastAdd   = graph.global->getTotalCalls() - 1;
                for (auto const& e : events)
                    CHECK((e.id == lastTwice || e.id == lastAdd));
            }
        }
    }
}

SCENARIO(R"(TracerSpy minDepth skips shallow events; stats still accumulate)")
{
    GIVEN(R"(a TracerSpy with minDepth = 1)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{{.minDepth = 1}},
            .main{}
        };

        WHEN(R"(twice invokes nested add)")
        {
            int const r = graph->doubler->asTrait(trait::doubler).twice(5);

            THEN(R"(only depth>=1 events recorded; both methods still in stats)")
            {
                CHECK(r == 10);
                auto const& events = graph.global->getEvents();
                CHECK(events.size() == 2);
                for (auto const& e : events)
                    CHECK(e.depth == 1);
                CHECK(graph.global->getStats().size() == 2);
                CHECK(graph.global->getTotalCalls() == 2);
            }
        }
    }
}

SCENARIO(R"(TracerSpy maxDepth skips deep events; stats still accumulate)")
{
    GIVEN(R"(a TracerSpy with maxDepth = 1)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{{.maxDepth = 1}},
            .main{}
        };

        WHEN(R"(twice invokes nested add)")
        {
            int const r = graph->doubler->asTrait(trait::doubler).twice(5);

            THEN(R"(only depth<1 (i.e. depth 0) events recorded; stats unchanged)")
            {
                CHECK(r == 10);
                auto const& events = graph.global->getEvents();
                CHECK(events.size() == 2);
                for (auto const& e : events)
                    CHECK(e.depth == 0);
                CHECK(graph.global->getStats().size() == 2);
                CHECK(graph.global->getTotalCalls() == 2);
            }
        }
    }
}


SCENARIO(R"(TracerSpy can pause and resume event recording on the fly)")
{
    GIVEN(R"(a TracerSpy with default Params)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        WHEN(R"(recording is stopped, calls are made, then recording is resumed)")
        {
            CHECK(graph.global->isRecording());

            graph.global->stopRecording();
            CHECK_FALSE(graph.global->isRecording());

            // 3 calls while paused: no events logged, but stats accumulate.
            for (int i = 0; i < 3; ++i)
                graph->doubler->asTrait(trait::doubler).twice(i);

            // Pause is a hard bypass: nothing logged, no stats, no id increment.
            CHECK(graph.global->getEvents().size() == 0);
            CHECK(graph.global->getTotalCalls() == 0);
            CHECK(graph.global->getStats().empty());

            graph.global->startRecording();
            CHECK(graph.global->isRecording());

            // 2 calls while recording: 4 events each (enter+exit for twice & add).
            graph->doubler->asTrait(trait::doubler).twice(10);
            graph->doubler->asTrait(trait::doubler).twice(20);

            THEN(R"(only events from the active window are recorded; stats only count recorded calls)")
            {
                CHECK(graph.global->getEvents().size() == 8);
                CHECK(graph.global->getTotalCalls() == 4);

                std::size_t twiceCalls = 0;
                std::size_t addCalls = 0;
                for (auto const& [method, stat] : graph.global->getStats())
                {
                    if (method.find("twice") != std::string_view::npos) twiceCalls = stat.calls;
                    else if (method.find("add") != std::string_view::npos) addCalls = stat.calls;
                }
                CHECK(twiceCalls == 2);
                CHECK(addCalls == 2);
            }
        }
    }
}

SCENARIO(R"(TracerSpy can be constructed paused via Params.recording = false)")
{
    GIVEN(R"(a TracerSpy constructed with recording = false)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{{.recording = false}},
            .main{}
        };

        WHEN(R"(calls are made before recording is explicitly enabled)")
        {
            CHECK_FALSE(graph.global->isRecording());

            graph->doubler->asTrait(trait::doubler).twice(1);
            graph->doubler->asTrait(trait::doubler).twice(2);

            THEN(R"(nothing is recorded while paused)")
            {
                CHECK(graph.global->getEvents().size() == 0);
                CHECK(graph.global->getTotalCalls() == 0);
                CHECK(graph.global->getStats().empty());
            }

            graph.global->startRecording();
            graph->doubler->asTrait(trait::doubler).twice(3);

            THEN(R"(events and stats appear only after startRecording() is called)")
            {
                CHECK(graph.global->getEvents().size() == 4);
                CHECK(graph.global->getTotalCalls() == 2);
                CHECK(graph.global->getStats().size() == 2);
            }
        }
    }
}


SCENARIO(R"(TracerSpy reset(Params) clears state and applies new Params)")
{
    GIVEN(R"(a TracerSpy that has recorded some events)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        graph->doubler->asTrait(trait::doubler).twice(1);
        graph->doubler->asTrait(trait::doubler).twice(2);
        CHECK(graph.global->getEvents().size() == 8);
        CHECK(graph.global->getTotalCalls() == 4);
        CHECK(graph.global->getStats().size() == 2);

        WHEN(R"(reset is called with new Params)")
        {
            graph.global->reset({.minDepth = 1, .maxEvents = 16});

            THEN(R"(all event/stats state is cleared and new Params are applied)")
            {
                CHECK(graph.global->getEvents().size() == 0);
                CHECK(graph.global->getEvents().begin_id() == 0);
                CHECK(graph.global->getEvents().end_id() == 0);
                CHECK(graph.global->getStats().empty());
                CHECK(graph.global->getTotalCalls() == 0);
                CHECK(graph.global->getMaxDepthSeen() == 0);
                CHECK(graph.global->getMinDepth() == 1);
                CHECK(graph.global->isRecording());
            }

            AND_WHEN(R"(more calls are made under the new config)")
            {
                graph->doubler->asTrait(trait::doubler).twice(9);

                THEN(R"(only depth>=1 events are recorded; stats reflect post-reset calls)")
                {
                    auto const& events = graph.global->getEvents();
                    CHECK(events.size() == 2);
                    for (auto const& e : events)
                        CHECK(e.depth == 1);
                    CHECK(graph.global->getTotalCalls() == 2);
                }
            }
        }

        WHEN(R"(reset is called with recording = false)")
        {
            graph.global->reset({.recording = false});

            THEN(R"(subsequent calls are not recorded)")
            {
                graph->doubler->asTrait(trait::doubler).twice(5);
                CHECK(graph.global->getEvents().size() == 0);
                CHECK(graph.global->getTotalCalls() == 0);
                CHECK_FALSE(graph.global->isRecording());
            }
        }
    }
}

SCENARIO(R"(TracerSpy enable()/disable() trait methods toggle recording and return true)")
{
    GIVEN(R"(a TracerSpy with default Params)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        auto spy = graph.global->asTrait(arc::spy);

        WHEN(R"(disable() is called via the trait interface)")
        {
            bool const disabled = spy.disable();

            THEN(R"(returns true and recording is off)")
            {
                CHECK(disabled == true);
                CHECK_FALSE(graph.global->isRecording());
            }

            AND_WHEN(R"(calls are made while disabled)")
            {
                graph->doubler->asTrait(trait::doubler).twice(1);

                THEN(R"(nothing is recorded)")
                {
                    CHECK(graph.global->getEvents().size() == 0);
                    CHECK(graph.global->getTotalCalls() == 0);
                }

                AND_WHEN(R"(enable() is called via the trait interface)")
                {
                    bool const enabled = spy.enable();

                    THEN(R"(returns true and recording resumes)")
                    {
                        CHECK(enabled == true);
                        CHECK(graph.global->isRecording());
                    }

                    AND_WHEN(R"(more calls are made)")
                    {
                        graph->doubler->asTrait(trait::doubler).twice(2);
                        graph->doubler->asTrait(trait::doubler).twice(3);

                        THEN(R"(only post-enable events are recorded)")
                        {
                            CHECK(graph.global->getEvents().size() == 8);
                            CHECK(graph.global->getTotalCalls() == 4);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO(R"(TracerSpy enable()/disable() are idempotent)")
{
    GIVEN(R"(a TracerSpy already recording)")
    {
        arc::GraphWithGlobal<cluster::Main, arc::TracerSpy, Root> graph{
            .global{},
            .main{}
        };

        auto spy = graph.global->asTrait(arc::spy);

        WHEN(R"(enable() is called when already recording)")
        {
            bool const r = spy.enable();

            THEN(R"(returns true and recording stays on)")
            {
                CHECK(r == true);
                CHECK(graph.global->isRecording());
            }
        }

        WHEN(R"(disable() is called twice)")
        {
            spy.disable();
            bool const r = spy.disable();

            THEN(R"(both return true and recording stays off)")
            {
                CHECK(r == true);
                CHECK_FALSE(graph.global->isRecording());
            }
        }
    }
}

} // namespace arc::tests::test_tracer_spy
