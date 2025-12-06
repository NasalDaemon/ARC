# Trait Spying in ARC

## Overview

The **Spy trait system** is a powerful meta-programming feature in ARC that allows you to intercept and observe trait method calls at runtime without modifying the implementation code. This enables transparent instrumentation for testing, debugging, profiling, security hardening, and runtime analysis.

## What is Trait Spying?

Trait spying works by intercepting calls to trait implementations through a special `intercept` method. When a global node implements `arc::trait::Spy` or `arc::trait::SpyOnly<Trait>`, the ARC framework automatically routes trait method invocations through the spy's intercept handler before executing the actual implementation.

Think of it as **Aspect-Oriented Programming (AOP)** for traits - you can inject cross-cutting concerns like logging, validation, or metrics collection without coupling these concerns to your business logic.

## Why Use Trait Spying?

### 1. **Testing & Verification**
- **Call Counting**: Track how many times methods are invoked
- **Argument Inspection**: Validate inputs before they reach the implementation
- **Behavior Modification**: Inject test-specific behaviors (e.g., fault injection)
- **Mock Verification**: Ensure expected interactions occur in integration tests

### 2. **Performance Analysis**
- **Profiling**: Measure execution time of trait methods
- **Hot Path Detection**: Identify frequently called operations
- **Performance Regression Detection**: Track performance metrics over time

### 3. **Debugging & Diagnostics**
- **Execution Tracing**: Log all trait method calls with arguments and results
- **State Inspection**: Examine system state at method boundaries
- **Debug Instrumentation**: Add temporary debug output without modifying source

### 4. **Security & Hardening**
- **Input Validation**: Sanitize or validate arguments before execution
- **Rate Limiting**: Throttle method calls to prevent abuse
- **Audit Logging**: Record sensitive operations for compliance
- **Access Control**: Enforce additional authorization checks

### 5. **Monitoring & Observability**
- **Metrics Collection**: Gather runtime statistics (call rates, error rates)
- **Distributed Tracing**: Integrate with tracing systems (OpenTelemetry, etc.)
- **Health Monitoring**: Track system health indicators

### 6. **Development & Maintenance**
- **Deprecation Warnings**: Warn about deprecated API usage
- **Migration Support**: Facilitate gradual API transitions
- **Feature Flags**: Conditionally modify behavior based on runtime configuration

## How Trait Spying Works

### The Interception Flow

When a spy exists in the context, trait calls are routed through `getGlobal(trait::spy).intercept(method, impl_fn, args...)`. The spy receives:
- **method**: The trait method tag (e.g., `trait::Storage::get`)
- **impl_fn**: The actual implementation (callable)
- **args**: The method arguments

The spy has complete control over execution:
- **Before**: Inspect/modify arguments, validate, log
- **During**: Wrap with timing, error handling, etc.
- **After**: Inspect/modify return values, collect metrics
- **Skip**: Optionally not call `impl_fn` at all

## Using Trait Spying

### Basic Setup: Spy All Traits

To intercept **all** trait method calls, implement `arc::trait::Spy`:

```cpp
struct GlobalSpy : arc::Node
{
    using Traits = arc::Traits<GlobalSpy, arc::trait::Spy>;

    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method method, auto impl_fn, Args&&... args)
    {
        // Called for every trait method invocation
        // `method` is the trait method tag (e.g., trait::Storage::get)
        // `impl_fn` is the actual implementation (callable)
        // `args` are the method arguments

        // Example: Count calls
        ++callCount;

        // Example: Log before execution
        std::println("Calling method: {}", arc::typeId<Method>);

        // Execute the actual implementation
        return impl_fn(std::forward<Args>(args)...);
    }

    int callCount = 0;
};
```

### Targeted Setup: Spy Specific Trait

To intercept only calls to a **specific trait**, use `arc::trait::SpyOnly<Trait>`:

```cpp
struct StorageSpy : arc::Node
{
    using Traits = arc::Traits<StorageSpy,
        arc::trait::SpyOnly<trait::Storage>
    >;

    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method method, auto impl_fn, Args&&... args)
    {
        // Only called for trait::Storage methods
        // More type-safe: Method is guaranteed to be a trait::Storage method

        if constexpr (std::same_as<Method, trait::Storage::get>)
        {
            // Specific handling for get operations
            std::println("Storage GET: {}", args...);
        }

        return impl_fn(std::forward<Args>(args)...);
    }
};
```

### Multiple Spy Traits

You can spy on multiple specific traits independently:

```cpp
struct MultiSpy : arc::Node
{
    struct StorageSpyImpl;
    struct FilesystemSpyImpl;

    using Traits = arc::Traits<MultiSpy,
        arc::trait::SpyOnly<trait::Storage>(StorageSpyImpl),
        arc::trait::SpyOnly<trait::Filesystem>(FilesystemSpyImpl)
    >;
};

struct MultiSpy::StorageSpyImpl : MultiSpy
{
    decltype(auto) impl(arc::trait::Spy::intercept, auto method, auto impl_fn, auto&&... args)
    {
        // Handles only trait::Storage methods
        return impl_fn(std::forward<decltype(args)>(args)...);
    }
};

struct MultiSpy::FilesystemSpyImpl : MultiSpy
{
    decltype(auto) impl(arc::trait::Spy::intercept, auto method, auto impl_fn, auto&&... args)
    {
        // Handles only trait::Filesystem methods
        return impl_fn(std::forward<decltype(args)>(args)...);
    }
};
```

### Integrating with Your Graph

Use `GraphWithGlobal` to inject the spy into your dependency graph:

```cpp
arc::GraphWithGlobal<MyCluster, GlobalSpy, MyRoot> graph{
    .global{}, // Spy node constructed here
    .main{
        // Your main graph configuration
    }
};

// All trait calls now route through the spy
auto result = graph->node->getNode(trait::storage).get("/path");

// Access spy state
std::println("Total calls: {}", graph.global->callCount);
```

## Practical Examples

### Example 1: Call Counting for Testing

```cpp
struct CallCounterSpy : arc::Node
{
    using Traits = arc::Traits<CallCounterSpy, arc::trait::Spy>;

    template<class Method>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, auto&&... args)
    {
        ++counts[arc::typeId<Method>];
        return impl_fn(std::forward<decltype(args)>(args)...);
    }

    std::map<arc::TypeId, int> counts;
};

TEST_CASE("Verify method call counts")
{
    arc::GraphWithGlobal<Cluster, CallCounterSpy> graph{...};

    // Run operations
    performOperations(graph);

    // Verify expectations
    CHECK(graph.global->counts[arc::typeId<trait::Storage::get>()] == 42);
}
```

### Example 2: Performance Profiling

```cpp
struct ProfilingSpy : arc::Node
{
    using Traits = arc::Traits<ProfilingSpy, arc::trait::Spy>;

    template<class Method>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, auto&&... args)
    {
        auto start = std::chrono::high_resolution_clock::now();

        // Execute and capture result (handles both void and non-void returns)
        if constexpr (std::is_void_v<decltype(impl_fn(args...))>)
        {
            impl_fn(std::forward<decltype(args)>(args)...);
            auto duration = std::chrono::high_resolution_clock::now() - start;
            recordTiming(arc::typeId<Method>, duration);
        }
        else
        {
            decltype(auto) result = impl_fn(std::forward<decltype(args)>(args)...);
            auto duration = std::chrono::high_resolution_clock::now() - start;
            recordTiming(arc::typeId<Method>, duration);
            return result;
        }
    }

    void recordTiming(arc::TypeId method, auto duration)
    {
        timings[method].push_back(duration);
    }

    std::map<arc::TypeId, std::vector<std::chrono::nanoseconds>> timings;
};
```

### Example 3: Argument Validation

```cpp
struct ValidationSpy : arc::Node
{
    using Traits = arc::Traits<ValidationSpy,
        arc::trait::SpyOnly<trait::Filesystem>
    >;

    decltype(auto) impl(arc::trait::Spy::intercept, trait::Filesystem::write, auto impl_fn, std::string_view path, std::string data)
    {
        // Validate path isn't empty
        if (path.empty())
            return std::unexpected(FsError::InvalidPath);

        // Sanitize data (remove null bytes)
        std::erase(data, '\0');

        // Call with sanitized arguments
        return impl_fn(path, std::move(data));
    }

    // Generic handler for other methods
    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, Args&&... args)
    {
        return impl_fn(std::forward<Args>(args)...);
    }
};
```

### Example 4: Debug Logging

```cpp
struct LoggingSpy : arc::Node
{
    using Traits = arc::Traits<LoggingSpy, arc::trait::Spy>;

    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, Args&&... args)
    {
        std::println("[TRACE] Entering: {}", arc::typeName<Method>);

        if constexpr (std::is_void_v<decltype(impl_fn(args...))>)
        {
            impl_fn(std::forward<Args>(args)...);
            std::println("[TRACE] Exiting: {} (void)", arc::typeName<Method>);
        }
        else
        {
            auto result = impl_fn(std::forward<Args>(args)...);
            std::println("[TRACE] Exiting: {} with result", arc::typeName<Method>);
            return result;
        }
    }
};
```

### Example 5: Conditional Behavior Modification

```cpp
struct TestingSpy : arc::Node
{
    using Traits = arc::Traits<TestingSpy,
        arc::trait::SpyOnly<trait::Storage>
    >;

    // Inject failures for testing error handling
    decltype(auto) impl(arc::trait::Spy::intercept, trait::Storage::get, auto impl_fn, std::string_view path)
    {
        // Simulate failure for specific test paths
        if (shouldFailNext && path == failPath)
        {
            shouldFailNext = false;
            return nullptr; // Simulate not found
        }

        return impl_fn(path);
    }

    template<class Method>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, auto&&... args)
    {
        return impl_fn(std::forward<decltype(args)>(args)...);
    }

    bool shouldFailNext = false;
    std::string failPath;
};
```

## Advanced Patterns

### Spy Composition

Combine multiple spy behaviors using inheritance or aggregation:

```cpp
struct CompositeSpy : arc::Node
{
    using Traits = arc::Traits<CompositeSpy, arc::trait::Spy>;

    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method method, auto impl_fn, Args&&... args)
    {
        // Layer 1: Logging
        if (enableLogging)
            logger.log(method);

        // Layer 2: Timing
        auto timer = profiler.startTiming(method);

        // Layer 3: Validation
        if (!validator.validate(method, args...))
            throw std::invalid_argument("Validation failed");

        // Execute
        return impl_fn(std::forward<Args>(args)...);
    }

    bool enableLogging = true;
    Logger logger;
    Profiler profiler;
    Validator validator;
};
```

### Conditional Spying

Enable/disable spying at runtime:

```cpp
struct ConditionalSpy : arc::Node
{
    using Traits = arc::Traits<ConditionalSpy, arc::trait::Spy>;

    template<class Method, class... Args>
    decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, Args&&... args)
    {
        if (!enabled)
            return impl_fn(std::forward<Args>(args)...);

        // Spy logic only when enabled
        recordCall<Method>();
        return impl_fn(std::forward<Args>(args)...);
    }

    bool enabled = true;
};
```

## Performance Considerations

### No Overhead

- **Zero cost when not used**: If no global spy is present in the graph, nothing is intercepted
- **No virtual dispatch**: Accessing the spy is just a normal `getGlobal(trait::spy).intercept(...)` call
- **Compile-time optimization**: The compiler can inline the `impl_fn` call within the spy, and eliminate overhead when the spy does minimal work

### Best Practices

1. **Use `SpyOnly<Trait>` when possible**: More specific types enable better compiler optimization
2. **Minimize work in hot paths**: Keep spy logic lightweight for frequently called methods
3. **Conditional compilation**: Use `#ifdef` or `if constexpr` to disable spying in production builds
4. **Lazy initialization**: Defer expensive spy setup until first use

```cpp
template<class Method, class... Args>
decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, Args&&... args)
{
    #ifdef ENABLE_PROFILING
    auto timer = startTimer();
    #endif

    return impl_fn(std::forward<Args>(args)...);
}
```

## Testing with Spies

The spy system is particularly valuable for testing:

```cpp
TEST_CASE("Filesystem caches path operations")
{
    struct CacheSpy : arc::Node
    {
        using Traits = arc::Traits<CacheSpy,
            arc::trait::SpyOnly<trait::PathOps>
        >;

        template<class Method, class... Args>
        decltype(auto) impl(arc::trait::Spy::intercept, Method, auto impl_fn, Args&&... args)
        {
            ++counts[arc::typeId<Method>];
            return impl_fn(std::forward<Args>(args)...);
        }

        std::map<arc::TypeId, int> counts;
    };

    arc::GraphWithGlobal<FilesystemCluster, CacheSpy> graph{...};

    // Perform operations
    graph->fs.read("/path/to/file");
    graph->fs.read("/path/to/file");  // Should use cached path
    graph->fs.read("/path/to/file");

    // Verify path normalization was called only once (cached)
    CHECK(graph.global->counts[arc::typeId<trait::PathOps::normalise>] == 1);
}
```

## Comparison with Alternatives

### vs. Mock Objects
- **Spies**: Observe real implementations, test actual behavior
- **Mocks**: Replace implementations, test in isolation
- **Use spies** when you want integration-level testing with observability

### vs. Direct Instrumentation
- **Spies**: Non-invasive, no modification to implementation code
- **Direct**: Tightly coupled, harder to enable/disable
- **Use spies** for cross-cutting concerns that shouldn't pollute business logic

### vs. Aspect-Oriented Programming (AOP)
- **Spies**: Type-safe, compile-time verified, zero overhead when unused
- **AOP**: More flexible, but runtime overhead and potential for runtime errors
- **Use spies** when working within the ARC framework's type system

## Limitations

1. **Trait methods only**: Can only intercept trait method calls via `getNode` and `asTrait`, not arbitrary function calls, such as calling `YourNode::impl(Method, ...)` directly
2. **Graph scope**: Only works within an ARC dependency graph with `GraphWithGlobal`

## Summary

The Spy trait system is a **powerful, type-safe, and performant** way to add observability and control to your ARC applications without modifying implementation code. It's particularly valuable for:

- **Testing**: Verify interactions without mocking
- **Debugging**: Trace execution in complex dependency graphs
- **Performance**: Profile method calls in production
- **Security**: Add validation and sanitization layers
- **Observability**: Integrate with monitoring systems

By leveraging compile-time type information and zero-cost abstractions, ARC's spy system provides the benefits of aspect-oriented programming while maintaining C++'s performance characteristics.
