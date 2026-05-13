# TracerSpy

`arc::TracerSpy` is a built-in global spy node that records every intercepted trait method call into an in-memory event log, then renders it on demand as JSONL or human-readable text. It is built on top of [`arc::trait::Spy`](spy-trait.md) and intended for debugging integration tests — especially [agentic workflows](#agentic-debugging) where an LLM consumes the trace — but is equally useful as a plain human tracing tool.

## Why TracerSpy

- **Lossless capture.** Every call, argument value (when formattable), return value, return type, timing, and exception is recorded as a structured `Event`. Output format is chosen at render time, never at construction.
- **Reconstructable tree.** Each event carries a stable `id`, `parent` id, `depth`, and the intercepted `node` handle. Render output is order-preserving, but downstream tools can rebuild the call tree and ground each event back to its cluster node without parsing indentation.
- **Two formats, one source.** Render the same recording as machine-friendly JSONL or human-readable indented text. No need to re-run the test.
- **Per-method summary.** A summary block (`evt: "summary"`) gives call counts, total + max time, and exception counts, enabling fast run-to-run regression comparison.
- **Zero formatting cost when silent.** Recording is cheap; rendering happens only when `write*` is called.
- **Compile-safe.** Argument and return values are best-effort formatted via the C++23 `std::formattable` concept; non-formattable types degrade gracefully to type name only.

## Quick start

```cpp
#include <arc/arc.hpp>      // or: import arc;
#include <iostream>

arc::GraphWithGlobal<cluster::App, arc::TracerSpy> graph{
    .global{},      // default-constructed; sink and format chosen at write time
    .main{ /* ... */ }
};

// Exercise the graph.
graph->api.asTrait(trait::request).handle("GET", "/items/42");

// Render the trace + per-method summary.
graph.global->write(std::cout, arc::TracerSpy::Format::Human);
```

The global spy intercepts every trait call automatically, so no further wiring is required beyond placing `TracerSpy` as the global node.

## Output formats

### Human (`Format::Human`)

```
-> #0 [app::node::Api] app::trait::Request::handle(std::string_view(GET), std::string_view(/items/42))
  -> #1 [app::node::Router] app::trait::Router::route(std::string_view(/items/42))
    -> #2 [app::node::Store] app::trait::Storage::get(int(42))
    <- #2 [app::node::Store] app::trait::Storage::get [1820 ns] -> Item = {id:42, name:"foo"}
  <- #1 [app::node::Router] app::trait::Router::route [4210 ns] -> int = 200
<- #0 [app::node::Api] app::trait::Request::handle [6900 ns] -> void
=== TracerSpy summary ===
  app::trait::Request::handle                                  calls=     1 exc=  0 total=6900ns max=6900ns
  app::trait::Router::route                                    calls=     1 exc=  0 total=4210ns max=4210ns
  app::trait::Storage::get                                     calls=     1 exc=  0 total=1820ns max=1820ns
  total_calls=3 max_depth=3
```

Indentation reflects call depth. The `[Node]` tag identifies the user-facing handle of the intercepted node (read from `Context::NodeHandle`). Each argument is rendered as `Type(value)` when the value is formattable, otherwise just `Type`. Pointers render as `Type(0xADDR -> value)` when the pointee is formattable, and `Type(nullptr)` for null pointers.

### JSONL (`Format::Jsonl`)

One JSON object per line. Easy to stream, easy to parse, easy to diff.

```jsonl
{"evt":"enter","id":0,"parent":0,"depth":0,"node":"app::node::Api","method":"app::trait::Request::handle","args":[{"type":"std::string_view","value":"GET"},{"type":"std::string_view","value":"/items/42"}]}
{"evt":"enter","id":1,"parent":0,"depth":1,"node":"app::node::Router","method":"app::trait::Router::route","args":[{"type":"std::string_view","value":"/items/42"}]}
{"evt":"enter","id":2,"parent":1,"depth":2,"node":"app::node::Store","method":"app::trait::Storage::get","args":[{"type":"int","value":"42"}]}
{"evt":"exit","id":2,"depth":2,"node":"app::node::Store","method":"app::trait::Storage::get","ns":1820,"ret_type":"Item","ret":"{id:42, name:\"foo\"}"}
{"evt":"exit","id":1,"depth":1,"node":"app::node::Router","method":"app::trait::Router::route","ns":4210,"ret_type":"int","ret":"200"}
{"evt":"exit","id":0,"depth":0,"node":"app::node::Api","method":"app::trait::Request::handle","ns":6900,"ret_type":"void"}
{"evt":"summary","method":"app::trait::Request::handle","calls":1,"exc":0,"total_ns":6900,"max_ns":6900}
{"evt":"summary","method":"app::trait::Router::route","calls":1,"exc":0,"total_ns":4210,"max_ns":4210}
{"evt":"summary","method":"app::trait::Storage::get","calls":1,"exc":0,"total_ns":1820,"max_ns":1820}
{"evt":"summary_meta","total_calls":3,"max_depth":3}
```

Event schema:

| Field | Where | Description |
|-------|-------|-------------|
| `evt` | all | `"enter"`, `"exit"`, `"throw"`, `"summary"`, `"summary_meta"` |
| `id` | enter/exit/throw | Unique, monotonically increasing call id |
| `parent` | enter | Caller's `id`, or `0` for top-level |
| `depth` | enter/exit/throw | Call depth (top-level = 0) |
| `node` | enter/exit/throw | Type name of the intercepted node handle (e.g. `app::node::Mirror`), read from the spy caller's `Context::NodeHandle` |
| `method` | enter/exit/throw | Fully-qualified trait method type name |
| `args` | enter | Array of `{type, value?}` records — `value` omitted when not `std::formattable` |
| `ns` | exit/throw | Wall-clock duration of the call in nanoseconds |
| `ret_type` | exit | Return type name (`"void"` for void returns) |
| `ret` | exit | Return value (omitted when not formattable) |
| `what` | throw | `std::exception::what()`, or `"<unknown>"` for non-`std::exception` throws |
| `calls`, `exc`, `total_ns`, `max_ns` | summary | Per-method aggregates |
| `total_calls`, `max_depth`, `events_emitted`, `events_in_buffer`, `events_evicted`, `events_capacity` | summary_meta | Whole-run aggregates. `events_evicted > 0` means the ring buffer dropped its oldest events; per-method stats are unaffected. |

## API

```cpp
namespace arc {

struct TracerSpy : Node
{
    using Traits = Traits<arc::trait::Spy>;

    enum class Format { Jsonl, Human };

    static constexpr std::size_t defaultMaxEvents = 32 * 1024;

    struct Params
    {
        std::size_t minDepth = 0;                 // Skip events at depth < minDepth (stats still accumulate).
        std::size_t maxDepth = 64;                // Skip events at depth >= maxDepth (stats still accumulate).
        std::size_t maxEvents = defaultMaxEvents; // Ring buffer capacity; oldest evict on overflow.
        bool        recording = true;             // Construct paused; toggle via start/stopRecording().
    };

    TracerSpy();
    explicit TracerSpy(Params p);

    void startRecording();          // Resume the bypassed call path.
    void stopRecording();           // Pause: calls are forwarded with zero recording or stats overhead.
    bool isRecording() const;

    // Discard events + stats, apply new Params (resizes ring + zeroes ids).
    void reset(Params p);
    // Same as reset() but keep current min/maxDepth + maxEvents;
    // optionally override recording (defaults to current value).
    void reset(std::optional<bool> recording = std::nullopt);

    // Read-only accessors. State is otherwise private.
    arc::CircularBuffer<Event> const& getEvents()       const;  // ring buffer; oldest entries evict on overflow
    std::map<std::string_view, Stats> const& getStats() const;  // per-method aggregates; survive eviction
    std::size_t getTotalCalls()    const;                       // every intercepted call, filtered or not
    std::size_t getMaxDepthSeen()  const;
    std::size_t getMinDepth()      const;
    std::size_t getMaxDepth()      const;

    // Render trace events followed by the summary block.
    void write(std::ostream& os, Format fmt) const;

    // Just the trace events.
    void writeTrace(std::ostream& os, Format fmt) const;

    // Just the summary block.
    void writeSummary(std::ostream& os, Format fmt) const;
};

} // namespace arc
```

`TracerSpy::Event` and `TracerSpy::Stats` are exposed so callers can post-process the recording (e.g. flame graphs, regression comparisons, custom formatters) without re-running the test.

## Recipes

### Per-test trace dump for diffing

```cpp
SCENARIO(R"(/users/me responds correctly)")
{
    arc::GraphWithGlobal<cluster::App, arc::TracerSpy> graph{ .global{}, .main{} };

    graph->api.asTrait(trait::request).handle("GET", "/users/me");

    std::ofstream out("users_me.trace.jsonl");
    graph.global->write(out, arc::TracerSpy::Format::Jsonl);
}
```

Commit the resulting trace as a golden file; subsequent runs diff against it.

### Render to a string for assertions

```cpp
std::ostringstream trace;
graph.global->write(trace, arc::TracerSpy::Format::Jsonl);
CHECK(trace.str().find("\"method\":\"app::trait::Storage::get\"") != std::string::npos);
```

### Tune recording window and buffer size via `Params`

```cpp
arc::GraphWithGlobal<cluster::App, arc::TracerSpy> graph{
    .global{{
        .minDepth = 1,        // skip top-level test-driver calls
        .maxDepth = 8,        // skip deep recursion
        .maxEvents = 16384,   // ring size; oldest evict on overflow
    }},
    .main{}
};
// Stats accumulate for every call regardless of min/maxDepth filters.
// `events_evicted` in summary_meta reports how many entries were dropped by the ring.
```

> **Golden-file traces:** when diffing JSONL output across runs, size `maxEvents` above the expected event count, otherwise eviction will shift the recorded prefix between runs and break the diff.

The event log is an `arc::CircularBuffer`, so each event also has a stable buffer slot id via the iterator's `id()` / `is_valid_id()` — useful when post-processing across multiple `write*` calls.

### Runtime toggle via the Spy trait

`enable()` and `disable()` are trait methods of `arc::trait::Spy`. They call `startRecording()`/`stopRecording()`, allowing arbitrary nodes in the graph to toggle spying. This can be helpful in reducing noise when only specific phases of execution are of interest.

```cpp
struct MyNode : arc::NodeImpl<MyTrait>
{
    void doSomething()
    {
        // ...
        getGlobal(arc::spy).enable();   // start recording from here
        // ...
        getGlobal(arc::spy).disable();  // stop recording from here
        // ...
    }
};
```

### Inspect events programmatically

```cpp
for (auto const& e : graph.global->getEvents())
    if (e.kind == arc::TracerSpy::Event::Kind::Throw)
        std::println("Exception in {}: {}", e.method, e.what);
```

## Agentic debugging

The JSONL format is designed to be consumed by an LLM driving an iterative test loop:

1. Agent edits a node and runs the test.
2. Agent reads the JSONL trace.
3. Agent diffs the `summary` block against the last green run — new exceptions, missing calls, and timing regressions surface immediately.
4. Agent walks the call tree to the lowest-depth divergent `id` to find the exact call chain and argument values that caused the regression.
5. Agent fixes the lowest-depth divergence — no spelunking, no human-in-loop.

Because each event carries both the trait-qualified `method` name and the user-facing `node` handle, the agent can ground every line directly in the corresponding `.ixx.arc` trait definition and the cluster wiring that placed the node.

## See also

- [Spy trait](spy-trait.md) — the underlying interception mechanism. `TracerSpy` is a turnkey `arc::trait::Spy` implementation; the spy-trait doc covers building custom spies and daisy-chaining them.
- [`/arc-node-builtins`](../.claude/skills/arc-node-builtins/SKILL.md) — quick-reference card of all built-in node wrappers.
