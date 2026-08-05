#pragma once

#include "arc/circular_buffer.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/traits/spy.hpp"
#include "arc/type_name.hpp"

#if !ARC_IMPORT_STD
#include <bit>
#include <chrono>
#include <cstddef>
#include <exception>
#include <format>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#endif

namespace arc {

// Construction parameters. All fields have sensible defaults so callers can
// brace-init with named arguments: `arc::TracerSpy{{.minDepth = 1}}`.
ARC_MODULE_EXPORT
struct TracerSpyParams
{
    // Skip recording events at depth < minDepth. Stats still accumulate.
    // Useful to suppress noisy top-level test scaffolding.
    std::size_t minDepth = 0;
    // Skip recording events at depth >= maxDepth. Stats still accumulate.
    std::size_t maxDepth = 64;
    // Ring buffer capacity for the event log; oldest entries evict on overflow.
    std::size_t maxEvents = 32 * 1024;
    // When false, event log is paused at construction.
    // Toggle via `startRecording()` / `stopRecording()`.
    bool recording = false;
};

// TracerSpy — global spy node that records every intercepted trait call
// losslessly into an in-memory event log. Output formatting (JSONL or
// human-readable) is chosen at write time, not at construction. Designed
// for debugging integration tests — especially agentic workflows where the
// trace is consumed by an LLM — but equally useful as a human tracing tool.
//
// Usage:
//   arc::GraphWithGlobal<cluster::App, arc::TracerSpy> graph{ .global{}, .main{...} };
//   // ... exercise the graph ...
//   graph.global->write(std::cout, arc::TracerSpy::Format::Human);
ARC_MODULE_EXPORT
struct TracerSpy : arc::Node
{
    using Traits = arc::Traits<arc::trait::Spy>;
    using Params = TracerSpyParams;

    enum class Format { Jsonl, Human };

    struct Arg
    {
        std::string_view type;  // points into static typeName storage
        std::string value;
        bool hasValue = false;
    };

    struct Event
    {
        enum class Kind { Enter, Exit, Throw };
        Kind kind = Kind::Enter;
        std::size_t id = 0;
        std::size_t parent = 0;
        std::size_t depth = 0;
        std::string_view method;
        std::string_view node;
        std::vector<Arg> args;          // Enter only
        std::chrono::nanoseconds ns{};  // Exit/Throw
        std::string_view retType;       // Exit only
        std::string retValue;           // Exit
        bool retHasValue = false;
        std::string what;               // Throw
    };

    struct Stats
    {
        std::size_t calls = 0;
        std::size_t exceptions = 0;
        std::chrono::nanoseconds total{};
        std::chrono::nanoseconds maxTime{};
    };

    explicit TracerSpy(Params p = {})
        : minDepthLog(p.minDepth)
        , maxDepthLog(p.maxDepth)
        , recording(p.recording)
        , events(p.maxEvents)
    {}

    // Pause/resume appending to the event ring. Stats accumulate regardless of
    // recording state. Resuming mid-call may yield orphan Enter/Exit pairs.
    void startRecording() { recording = true; }
    void stopRecording()  { recording = false; }
    bool isRecording() const { return recording; }

    // Discard all recorded events and stats, then apply new Params. Useful
    // between test phases to scope tracing to a single window without
    // tearing down the graph.
    void reset(Params p);
    // Reset but preserve params
    void reset(std::optional<bool> recording = std::nullopt)
    {
        reset({
            .minDepth = minDepthLog,
            .maxDepth = maxDepthLog,
            .maxEvents = events.max_size(),
            .recording = recording.value_or(this->recording)
        });
    }

    arc::CircularBuffer<Event> const& getEvents() const { return events; }
    std::map<std::string_view, Stats> const& getStats() const { return perMethod; }
    std::size_t getTotalCalls() const { return nextCallId; }
    std::size_t getMaxDepthSeen() const { return maxDepthSeen; }
    std::size_t getMinDepth() const { return minDepthLog; }
    std::size_t getMaxDepth() const { return maxDepthLog; }

    bool impl(Spy::enable) { startRecording(); return true; }
    bool impl(Spy::disable) { stopRecording(); return true; }

    template<class Method, class Caller, class... Args>
    decltype(auto) impl(Spy::intercept, Method, Caller const& impl_fn, Args&&... args) const
    {
        // Hard bypass when paused: no id, no stats, no stack, no events.
        // Caller observes a pure forward to the underlying implementation.
        if (!recording)
            return impl_fn(std::forward<Args>(args)...);

        auto& stack = parentStack();
        std::size_t const d = stack.size();
        std::size_t const id = nextCallId++;
        std::size_t const parent = d > 0 ? stack.back() : 0;

        std::string_view const name = arc::typeName<Method>;
        std::string_view const nodeName = arc::typeName<typename Caller::NodeHandle>;;
        bool const recordThis = d >= minDepthLog && d < maxDepthLog;

        if (recordThis)
        {
            Event e;
            e.kind = Event::Kind::Enter;
            e.id = id;
            e.parent = parent;
            e.depth = d;
            e.method = name;
            e.node = nodeName;
            (appendArg(e.args, args), ...);
            events.push_back(std::move(e));
        }

        stack.push_back(id);
        if (stack.size() > maxDepthSeen)
            maxDepthSeen = stack.size();

        auto& stat = perMethod[name];
        ++stat.calls;
        auto const t0 = std::chrono::steady_clock::now();

        struct Guard
        {
            std::vector<std::size_t>& s;
            ~Guard() { if (!s.empty()) s.pop_back(); }
        } guard{stack};

        try
        {
            using R = decltype(impl_fn(std::forward<Args>(args)...));
            if constexpr (std::is_void_v<R>)
            {
                impl_fn(std::forward<Args>(args)...);
                auto dt = recordElapsed(stat, t0);
                if (recordThis) recordExit(id, d, name, nodeName, dt, "void", {}, false);
            }
            else
            {
                decltype(auto) r = impl_fn(std::forward<Args>(args)...);
                auto dt = recordElapsed(stat, t0);
                if (recordThis)
                {
                    std::string_view const retType = arc::typeName<std::remove_cvref_t<R>>;
                    std::string const retVal = formatValue(r);
                    recordExit(id, d, name, nodeName, dt, retType, retVal, !retVal.empty());
                }
                if constexpr (std::is_rvalue_reference_v<R>)
                    return std::move(r);
                else
                    return r;
            }
        }
        catch (std::exception const& e)
        {
            auto dt = recordElapsed(stat, t0);
            ++stat.exceptions;
            if (recordThis) recordThrow(id, d, name, nodeName, dt, e.what());
            throw;
        }
        catch (...)
        {
            auto dt = recordElapsed(stat, t0);
            ++stat.exceptions;
            if (recordThis) recordThrow(id, d, name, nodeName, dt, "<unknown>");
            throw;
        }
    }

    // Full report: trace events followed by summary block.
    void write(std::ostream& os, Format fmt) const;
    void writeTrace(std::ostream& os, Format fmt) const;
    void writeSummary(std::ostream& os, Format fmt) const;

private:
    std::size_t minDepthLog{};
    std::size_t maxDepthLog{};
    bool recording{};

    // Bounded ring buffer: oldest events are evicted when capacity is reached.
    // Per-method `Stats` are independent and accumulate across evictions, so the
    // summary remains accurate even when the trace itself has been truncated.
    mutable arc::CircularBuffer<Event> events;
    mutable std::map<std::string_view, Stats> perMethod;
    mutable std::size_t nextCallId = 0;
    mutable std::size_t maxDepthSeen = 0;

    static std::vector<std::size_t>& parentStack();

    std::chrono::nanoseconds recordElapsed(Stats& stat, std::chrono::steady_clock::time_point t0) const noexcept
    {
        auto const dt = std::chrono::steady_clock::now() - t0;
        stat.total += dt;
        if (dt > stat.maxTime) stat.maxTime = dt;
        return dt;
    }
    void recordExit(std::size_t id, std::size_t d, std::string_view name, std::string_view nodeName,
                    std::chrono::nanoseconds dt,
                    std::string_view retType, std::string retValue, bool hasValue) const;
    void recordThrow(std::size_t id, std::size_t d, std::string_view name, std::string_view nodeName,
                     std::chrono::nanoseconds dt, std::string what) const;

    void writeEvent(std::ostream& os, Event const& e, Format fmt) const;
    void writeEventJsonl(std::ostream& os, Event const& e) const;
    void writeEventHuman(std::ostream& os, Event const& e) const;

    template<class T>
    static std::string formatValue(T const& v) noexcept
    {
        try
        {
            using Raw = std::remove_cvref_t<T>;
            if constexpr (std::is_same_v<Raw, std::nullptr_t>)
            {
                return "nullptr";
            }
            else if constexpr (std::is_pointer_v<Raw>)
            {
                if (!v) return "nullptr";
                using Pointee = std::remove_cv_t<std::remove_pointer_t<Raw>>;
                if constexpr (!std::is_void_v<Pointee>
                           && !std::is_function_v<Pointee>
                           && std::formattable<Pointee, char>)
                    return std::format("0x{:x} -> {}", std::bit_cast<std::uintptr_t>(v), *v);
                else
                    return std::format("0x{:x}", std::bit_cast<std::uintptr_t>(v));
            }
            else if constexpr (std::formattable<Raw, char>)
            {
                return std::format("{}", v);
            }
            else
            {
                return {};
            }
        }
        catch (...)
        {
            return "<?>";
        }
    }

    template<class T>
    static void appendArg(std::vector<Arg>& out, T const& a)
    {
        Arg arg;
        arg.type = arc::typeName<std::remove_cvref_t<T>>;
        arg.value = formatValue(a);
        arg.hasValue = !arg.value.empty();
        out.push_back(std::move(arg));
    }

    static std::string jsonEscape(std::string_view s);
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::TracerSpy;
}

}
