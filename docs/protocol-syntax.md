# Protocol syntax

Most interfaces have a lifecycle. A network connection is opened before data flows and closed before the object is destroyed. A file is not written after it is finalised. A transaction is not committed twice. These constraints are real — they are part of the interface's contract — yet in most codebases they live only in comments and developer folklore, enforced by nothing stronger than convention and hope.

Without explicit lifecycle contracts, three failure modes appear reliably:
- Code that calls methods in the wrong order is not caught at the call site; it propagates corrupted state until something fails downstream (if you are lucky).
- Multiple implementations each replicate the same lifecycle logic independently, so one will eventually drift from the others.
- Interface users have no signal about what ordering is required and must read an implementation to find out.

A protocol makes the lifecycle a first-class part of the trait definition. It declares which states exist, which transitions are legal, and what must hold at every call boundary. The framework enforces all of this at runtime under contract assertions, catching violations at the call site rather than letting them propagate.

## A motivating example

Here is a network connection trait with a protocol. The protocol declares the legal lifecycle of the connection and each channel it manages; each method in the trait declares precisely what transition it makes and under what conditions.

```arc
namespace app {

protocol Connection {
    Closed <-> Open -> Error -> Dead
    Closed <---------- Error

    per(std::string name) {
        state Channel {
            Disconnected --> Connecting --> Connected
            Disconnected <-- Connecting | Connected
            Connected implies Open   // a connected channel implies the connection is open
        }
    }

    hasPendingData(std::string name) implies Connected(name)
    isDead iff Dead
}

trait Network [Protocol = protocol::Connection] {
    open() -> bool
        is Closed then (success: success == proto.Open())
    connect(std::string name) -> bool
        is Open
        if (proto.Disconnected(name)) then (success: success ? proto.Connecting(name) : proto.Disconnected(name))
        if (proto.Connecting(name)) then (success: success ? proto.Connected(name) : proto.Disconnected(name))
    close() -> bool
        is Open then (success: success == proto.Closed())
    kill() -> void
        is Error then Dead
    reset() -> bool
        to Closed
    read(std::string name) -> std::string
        is hasPendingData(name)
}

} // namespace app
```

Reading this trait definition, a caller knows the full lifecycle without reading an implementation:

- `open()` can only be called in `Closed`. If it returns `true`, the state is `Open`; if `false`, it stays `Closed`.
- `connect()` can only be called while `Open`. Each call advances one handshake step — `Disconnected → Connecting → Connected` — or rolls back to `Disconnected` on failure.
- `read()` can only be called on a channel that `hasPendingData`, and `hasPendingData` can only return `true` while that channel is `Connected`. A caller cannot read from a channel that is not connected.
- `reset()` leaves the connection `Closed` from any state — but calling it from `Dead` is a bug: `Dead → Closed` is not a declared transition and the framework asserts at the call site.
- Any implementation that silently skips a transition, or transitions to a state not covered by the contract, is caught immediately at the boundary where the mismatch occurred.

## Quick reference

| Syntax | Description |
|--------|-------------|
| `A --> B` | One-way transition A → B |
| `A <-> B` | Bidirectional A ↔ B |
| `A \| B` | Transition source/target set |
| `state Name { ... }` | Named state group |
| `per(T p, ...) { ... }` | Parameterised per-key group block |
| `V implies Expr` | Variant invariant: V active implies Expr holds |
| `V iff Expr` | Variant invariant: V active iff Expr holds |
| `name implies Expr` | Predicate: return true implies Expr holds |
| `name iff Expr` | Predicate: return true iff Expr holds |
| `[Protocol = protocol::P]` | Bind a trait to protocol P |
| `to State` | Post-call: state must equal State |
| `is Pre` | Pre-call: state must equal Pre; exit state unconstrained |
| `is Pre then Post` | Pre must hold on entry; Post must hold on exit |
| `if Cond then Result` | If Cond held on entry, Result must hold on exit |

## Defining a protocol

Protocols are defined in `.arc` files alongside traits. They live inside `namespace protocol` within the enclosing namespace — `protocol Connection` inside `namespace app` produces `app::protocol::Connection`.

```arc
namespace app {

protocol Connection {
    // Top-level transitions — group named "Main", enum: States::Main
    Closed <-> Open -> Error -> Dead
    Closed <---------- Error

    // Named group — enum: States::Channel
    state Channel {
        Disconnected --> Connecting --> Connected
        Disconnected <-- Connecting | Connected
    }

    // Per-key parameterised group — one instance per distinct key
    // Accessor: protoStream(name); variants: Idle(name), Buffering(name), Streaming(name)
    per(std::string name) {
        state Stream {
            Idle --> Buffering --> Streaming
            Idle <-- Buffering | Streaming
        }
    }
}

} // namespace app
```

### Main group naming

| Where declared | Group name | Enum type |
|----------------|------------|-----------|
| Directly in protocol body | `Main` | `States::Main` |
| `state Name { ... }` | `Name` | `States::Name` |
| Inside `per(...)` | as declared | `States::Name`, keyed by parameter |

### Transition arrows

| Arrow | Meaning |
|-------|---------|
| `A --> B` | A → B |
| `A <-- B` | A ← B (equivalent to B → A) |
| `A <-> B` | A ↔ B (bidirectional) |

Arrows chain: `A --> B --> C` adds A→B and B→C.

Main sets work on either side: `A | B --> C` adds A→C and B→C. `A --> B | C` adds A→B and A→C.

### Default state and reachability

The first state mentioned anywhere in the protocol is the default state. Every state must be reachable from the default — the generator rejects unreachable states at build time, so the transition diagram cannot describe an impossible path.

Self-transitions are always valid: a method that does not change state is not rejected as long as it conforms to the method's protocol contract. States not reachable from the default have no legal entry path and cannot exist at runtime.

## Variant invariants

Real systems often have multiple groups of state that must stay consistent with each other. A channel can only be connected while the parent connection is open. A TLS session must be established exactly when its channel is connected. These cross-group constraints are just as much a part of the contract as the transition tables themselves, yet without a protocol they live only in documentation and are independently re-verified (or forgotten) in each implementation.

Variant invariants declare these constraints after the transition lines inside a `state` or `per` block. The framework checks them before and after every trait method call, so any violation is caught at the boundary where it first occurred.

```arc
protocol Connection {
    Closed <-> Open -> Error -> Dead
    Closed <---------- Error

    per(std::string name) {
        state Channel {
            Disconnected --> Connecting --> Connected
            Disconnected <-- Connecting | Connected

            Connected implies Open          // one-way: a connected channel requires an open connection
                                            // the connection can be open with no channels connected
        }

        state Encryption {
            Plain <-> Negotiating <-> Secured

            Secured iff Connected(name)     // two-way: TLS is secured exactly when the channel is connected
                                            // neither can be true without the other
        }
    }
}
```

- **`implies`** — one-way: if the source variant is active the target expression must hold. The converse is not required. Use this when one state is a necessary precondition for another, but the inverse does not hold.
- **`iff`** — two-way: the source variant is active if and only if the target expression holds. Both directions are verified simultaneously. Use this when two groups must always be in lockstep — any divergence in either direction is a bug.

The checks performed:
- `implies`: `!source || target`
- `iff`: `source == target`

The source/target expression can be:
- A bare variant name (`Open`) — expanded to `proto.Open()`.
- A parameterised call (`Connected(name)`) — expanded to `proto.Connected(name)`.
- A parenthesised C++ expression (`(proto.Open() || proto.Error())`) — used as-is.

## Predicate methods

Not every meaningful query maps directly onto a state variant. Callers often need derived boolean queries: "does this channel have data ready to read?", "has this connection reached a terminal state?". Without a protocol, these are ad-hoc methods returning `bool` with their relationship to the underlying state documented nowhere, verified nowhere, and frequently inconsistent between implementations.

Protocol predicates give these methods the same first-class status as state transitions. A predicate declaration specifies the logical relationship between the method's return value and the state machine. The framework checks the declared relationship after every call.

```arc
protocol Connection {
    Closed <-> Open -> Error -> Dead
    Closed <---------- Error

    per(std::string name) {
        state Channel {
            Disconnected --> Connecting --> Connected
            Connected implies Open
        }
    }

    // Returning true is only valid while the channel is Connected — checked on every call exit.
    // Returning false when the channel is Connected is permitted (data may not have arrived yet).
    hasPendingData(std::string name) implies Connected(name)

    // The return value must exactly mirror whether the state is Dead.
    // Returning false when in Dead, or true when not in Dead, is caught immediately.
    isDead iff Dead

    // Parameterised predicate — per-channel query
    isEstablished(std::string name) iff Connected(name)
}
```

- **`implies`** — an upper bound. Returning `true` is only valid in certain states; returning `false` carries no obligation. Use this for "can only be true when..." relationships.
- **`iff`** — a faithful mirror. Returning `true` when the state says otherwise, or `false` when the state says true, is caught immediately. Use this when the predicate is supposed to accurately report state.

The generated checks (applied after every call that invokes the predicate):
- `implies`: `!return_value || bool(expr)`
- `iff`: `return_value == bool(expr)`

The target expression follows the same rules as variant invariant targets: bare name, `Name(args)`, or `(cpp_expr)`.

Predicates are accessible via `proto.name(args...)` inside trait contracts:

```arc
trait Network [Protocol = protocol::Connection] {
    read(std::string name) -> std::string
        is hasPendingData(name)    // proto.hasPendingData(name) must hold on entry
                                   // no exit-state clause → state must not change
}
```

## Binding a protocol to a trait

Annotate a trait with `[Protocol = protocol::Name]` to attach its state machine. Inside contracts, `proto` refers to a const view of the protocol — state is read but never written through it.

### Contract clauses

| Clause | Entry check | Exit check |
|--------|-------------|------------|
| `to State` | none | state must equal `State` |
| `is Pre` | `Pre` must hold | none at the contract level; protocol constraints still apply |
| `is Pre then Post` | `Pre` must hold | `Post` must hold |
| `is Pre then (r: expr)` | `Pre` must hold | `expr` must hold (return value bound to `r`) |
| `if Cond then (r: expr)` | none | if `Cond` held on entry, `expr` must hold |

`to Closed` is shorthand for `to (proto.Closed())`. `is Open then Error` is shorthand for `is (proto.Open()) then (proto.Error())`. Parameterised references such as `to Connected(name)` expand to `to (proto.Connected(name))`.

Multiple clauses can appear on a single method and are evaluated independently:

```arc
trait Network [Protocol = protocol::Connection] {
    connect(std::string name) -> bool
        is Open                            // nullary Main must be Open on entry
        if Disconnected(name) then (success: success ? proto.Connecting(name) : proto.Disconnected(name))
        if Connecting(name) then (success: success ? proto.Connected(name) : proto.Disconnected(name))
}
```

A method with no exit-constraining clause (`to`, `then`, or `if ... then`) must not change any state. Any unannounced transition is caught.

When there are exit-constraining clauses, the framework allows state changes but still checks that all transitions that do happen have been declared and are active. For example: given `close() -> bool to (r: not r or proto.Closed())`, if `close()` retuns `false`, then no state change of any kind is allowed, since no protocol state is checked when `r` is false.

### Transition validity

Every state change made by a trait method must correspond to a declared transition in the protocol, or the framework asserts. This is checked in addition to any explicit contract clause — a method that satisfies `to Closed` but arrives there via an undeclared path still fails.

Calling `reset() -> bool to Closed` from `Dead` state asserts: `Dead → Closed` is not a declared transition in the `Connection` protocol. The contract cannot override the transition table.

### Protocol view methods

For each state group `G` (nullary or parameterised) the generator produces:

| Method | Returns | Description | Defined in implementing node |
|--------|---------|-------------|------|
| `proto.protoG(args...)` | `States::G` | Current enum value | Yes |
| `proto.Variant(args...)` | `bool` | True if the group is in that variant | No, auto-generated in protocol trait view |

All protocol predicates are also accessible as `proto.name(args...)`.

## Implementing a protocol-bound trait

The implementing node owns the state and exposes it via const accessor methods. The framework calls these before and after every trait method call — recording state before the call, comparing after, validating that any transition is declared, and checking all contract clauses, variant invariants, and predicate return values. The node's method implementations update the state freely; no manual validation is required.

Both the trait and the protocol must be listed in `arc::NodeImpl`:

```cpp
namespace node {

using States  = arc::States<protocol::Connection>;
using Main    = States::Main;
using Channel = States::Channel;

struct Network : arc::NodeImpl<trait::Network, protocol::Connection>
{
    // Main accessors — const, called by the framework to read state before and after every call

    Main protoState() const { return state; }

    Channel protoChannel(std::string const& name) const
    {
        auto it = channels.find(name);
        return it != channels.end() ? it->second : Channel::Disconnected;
    }

    // Predicate methods — const, return value checked against the declared implies/iff constraint

    bool hasPendingData(std::string const& name) const
    {
        auto it = pending.find(name);
        return it != pending.end() && !it->second.empty();
    }

    bool isDead() const { return state == Main::Dead; }

    bool isEstablished(std::string const& name) const
    {
        auto it = channels.find(name);
        return it != channels.end() && it->second == Channel::Connected;
    }

    // Trait methods — mutable, update state; the framework validates every transition and clause

    bool open()
    {
        state = Main::Open;
        return true;
    }

    bool connect(std::string name)
    {
        auto& ch = channels[name];
        if      (ch == Channel::Disconnected) ch = Channel::Connecting;
        else if (ch == Channel::Connecting)   ch = Channel::Connected;
        return true;
    }

    std::string read(std::string name)
    {
        auto& data = pending[name];
        std::string result = std::move(data);
        data.clear();
        return result;
    }

    bool close()
    {
        channels.clear();
        pending.clear();
        state = Main::Closed;
        return true;
    }

    void kill() { state = Main::Dead; }

    bool reset()
    {
        channels.clear();
        pending.clear();
        state = Main::Closed;
        return true;
    }

    Main state = Main::Closed;
    std::map<std::string, Channel> channels;
    std::map<std::string, std::string> pending;
};

} // namespace node
```

### What the framework checks on every call

| Moment | Check |
|--------|-------|
| Entry | All invariants hold |
| Entry | Pre-condition clauses (`is Pre`, `is Pre then Post`) |
| Entry | Predicate entry conditions (`is predicate(args)`) |
| Entry | Entry state is recorded for conditional post-checks (`if Cond then ...`) |
| Exit | Post-condition clauses (`then Post`, `to State`, `if Cond then Result`) |
| Exit | All predicate return values satisfy their declared constraint |
| Exit | Every state change follows a declared transition |
| Exit | Every state transition has been asserted by the contract |
| Exit | All invariants hold |

The state accessor methods add no overhead when assertions are disabled — they are plain `const` methods that the framework calls only during assertion evaluation. The node is free to store state in any form; the accessors can perform any necessary translation to produce the correct enum value.

### Enforcement scope

Protocol checks are applied at the boundary of each protocol-bound trait method call. A node may implement multiple traits; only the trait annotated with `[Protocol = ...]` is subject to protocol enforcement. Any other trait implemented by the same node can freely call mutable methods on the node's own state — including the fields that protocol accessors read — without passing through the protocol checks.

This is intentional: the protocol expresses the contract of *that specific trait*, with each method call independently verified, not a global invariant of the node. If two traits share state and their orderings interact, the correct model is to declare a protocol broad enough to cover both, and attach the protocol to both traits, or to separate the concerns into distinct nodes.

The protocol state can be queried independently at any time using `asTrait(arc::protocol(trait))` on the trait handle:

```cpp
auto proto = getNode(trait::network).asTrait(arc::protocol(trait::network));
bool isOpen = view.Open();  // true if the connection is currently Open
```

## Generated C++ API

```cpp
using S = arc::States<app::protocol::Connection>;

S::Main   s  = S::Main::Closed;              // top-level Main group
S::Channel ch = S::Channel::Disconnected;    // named Channel group

bool ok  = isValidTransition(S::Main::Closed, S::Main::Open); // true
bool bad = isValidTransition(S::Main::Dead,   S::Main::Open); // false

std::string_view name = toString(S::Main::Open); // "Open"

static_assert(arc::IsProtocol<app::protocol::Connection>);
static_assert(arc::HasProtocol<app::trait::Network>);
```

### Querying protocol state at runtime

Any trait handle exposes `asTrait(arc::protocol(trait))`, which returns a const view of the bound protocol. All state variant checkers and predicate methods are accessible through the view:

```cpp
auto proto = graph.network.asTrait(trait::network).asTrait(arc::protocol(trait::network));

// Nullary state
bool isOpen  = proto.Open();   // true if in Open state
bool isClosed = proto.Closed(); // true if in Closed state

// Parameterised group
bool isConnected = proto.Connected("channelA"); // true if channelA is Connected

// Predicate methods
bool hasPending = proto.hasPendingData("channelA");
bool dead       = proto.isDead();

// Raw enum value
S::Main   s  = proto.protoState();
S::Channel ch = proto.protoChannel("channelA");
```

`asTrait(arc::protocol(trait))` does not trigger any `trait::Network` protocol checks.
