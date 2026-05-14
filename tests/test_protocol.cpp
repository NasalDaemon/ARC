#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <map>
#include <string>
#if ARC_COMPILER_GCC
#include <typeinfo>
#endif
#endif

import arc.tests.protocol_test;
import arc;

#include "doctest.h"

namespace arc::tests::protocol_test {

/* arc-begin

export module arc.tests.protocol_test;

namespace arc::tests::protocol_test {

protocol Proto {
    Closed <-> Open -> Error -> Dead
    Closed <---------- Error

    per(std::string name) {
        state Named {
            Disconnected <-- Connecting | Connected
            Disconnected --> Connecting --> Connected

            Connected implies Open
        }
    }
    hasPendingData(std::string name) implies (proto.Connected(name))
    isDead iff Dead
}

trait Connections [Protocol = protocol::Proto] {
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
    forceOpen() -> void
        is Closed then Open
    read(std::string name) -> std::string
        // no state change announced, no state change allowed
        is hasPendingData(name)
}

protocol IffProto {
    state P {
        Off <-> On
        On iff High
    }
    state Q {
        Low <-> High
    }

    per(std::string k) {
        state R {
            Dim <-> Lit
            Lit iff Hot
        }
        state S {
            Cold <-> Hot
        }
    }

    isOn iff On
    isLit(std::string k) iff Lit(k)
}

trait IffTrait [Protocol = protocol::IffProto] {
    setOn()   -> void to On
    setOff()  -> void to Off
    setHigh() -> void to High
    setLow()  -> void to Low

    setLit(std::string k)  -> void to Lit(k)
    setDim(std::string k)  -> void to Dim(k)
    setHot(std::string k)  -> void to Hot(k)
    setCold(std::string k) -> void to Cold(k)
}

}

arc-end */

// ===========================================================================
// Aliases and static checks
// ===========================================================================

using States = arc::States<protocol::Proto>;
using Main   = States::Main;
using Named  = States::Named;

using IffStates = arc::States<protocol::IffProto>;
using P = IffStates::P;
using Q = IffStates::Q;
using R = IffStates::R;
using S = IffStates::S;

static_assert( arc::IsProtocol<protocol::Proto>);
static_assert(!arc::IsProtocol<trait::Connections>);
static_assert( arc::HasProtocol<trait::Connections>);
static_assert( arc::IsProtocol<protocol::IffProto>);
static_assert( arc::HasProtocol<trait::IffTrait>);

static_assert(toString(Main::Closed) == "Closed");
static_assert(toString(Main::Open)   == "Open");
static_assert(toString(Main::Error)  == "Error");
static_assert(toString(Main::Dead)   == "Dead");
static_assert(toString(Named::Disconnected) == "Disconnected");
static_assert(toString(Named::Connecting)   == "Connecting");
static_assert(toString(Named::Connected)    == "Connected");

static_assert(arc::IsMethodOf<protocol::Proto::Closed,          protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Open,            protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Error,           protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Dead,            protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Disconnected,    protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Connecting,      protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::Connected,       protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::protoMain,       protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::protoNamed,      protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::hasPendingData,  protocol::Proto>);
static_assert(arc::IsMethodOf<protocol::Proto::isDead,          protocol::Proto>);

// ===========================================================================
// Caller nodes
// ===========================================================================

struct Caller : arc::NodeUses<trait::Connections>
{
    bool        open      (this auto& s)                 { return s.getConnections().open(); }
    bool        connect   (this auto& s, std::string n)  { return s.getConnections().connect(std::move(n)); }
    bool        close     (this auto& s)                 { return s.getConnections().close(); }
    void        kill      (this auto& s)                 {        s.getConnections().kill(); }
    bool        reset     (this auto& s)                 { return s.getConnections().reset(); }
    void        forceOpen (this auto& s)                 {        s.getConnections().forceOpen(); }
    std::string read      (this auto& s, std::string n)  { return s.getConnections().read(std::move(n)); }
};

struct ProtoCaller : arc::NodeUses<protocol::Proto>
{
    bool hasData(this auto& s, std::string n) { return s.getProto().hasPendingData(std::move(n)); }
};

struct IffCaller : arc::NodeUses<trait::IffTrait>
{
    void setOn  (this auto& s)                { s.getIffTrait().setOn();  }
    void setOff (this auto& s)                { s.getIffTrait().setOff(); }
    void setHigh(this auto& s)                { s.getIffTrait().setHigh();}
    void setLow (this auto& s)                { s.getIffTrait().setLow(); }
    void setLit (this auto& s, std::string k) { s.getIffTrait().setLit (std::move(k)); }
    void setDim (this auto& s, std::string k) { s.getIffTrait().setDim (std::move(k)); }
    void setHot (this auto& s, std::string k) { s.getIffTrait().setHot (std::move(k)); }
    void setCold(this auto& s, std::string k) { s.getIffTrait().setCold(std::move(k)); }
};

struct IffProtoCaller : arc::NodeUses<protocol::IffProto>
{
    bool isOn (this auto& s)                { return s.getIffProto().isOn(); }
    bool isLit(this auto& s, std::string k) { return s.getIffProto().isLit(std::move(k)); }
};

using Mocks         = arc::test::Mock<>;
using Graph         = arc::test::Graph<Caller,         Mocks>;
using ProtoGraph    = arc::test::Graph<ProtoCaller,    Mocks>;
using IffGraph      = arc::test::Graph<IffCaller,      Mocks>;
using IffProtoGraph = arc::test::Graph<IffProtoCaller, Mocks>;

// ===========================================================================
// Shared mock state
// ===========================================================================

struct MockState
{
    Main nullary = Main::Closed;
    std::map<std::string, Named> keyed;

    template<class G>
    explicit MockState(G& g)
    {
        g.mocks->defineConst(
            [&](protocol::Proto::protoMain) -> Main { return nullary; },
            [&](protocol::Proto::protoNamed, std::string n) -> Named {
                auto it = keyed.find(n);
                return it != keyed.end() ? it->second : Named::Disconnected;
            },
            [&](protocol::Proto::hasPendingData, std::string n) -> bool {
                auto it = keyed.find(n);
                return it != keyed.end() && it->second == Named::Connected;
            },
            [&](protocol::Proto::isDead) -> bool { return nullary == Main::Dead; });
        if constexpr (std::same_as<G, Graph>)
        {
            g.mocks->define(
                [&](trait::Connections::open)      -> bool { nullary = Main::Open;   return true; },
                [&](trait::Connections::close)     -> bool { nullary = Main::Closed; return true; },
                [&](trait::Connections::kill)              { nullary = Main::Dead;   },
                [&](trait::Connections::reset)     -> bool { nullary = Main::Closed; return true; },
                [&](trait::Connections::forceOpen)         { nullary = Main::Open;   },
                [&](trait::Connections::connect, std::string n) -> bool {
                    auto it = keyed.find(n);
                    if (it == keyed.end() || it->second == Named::Disconnected) {
                        keyed[n] = Named::Connecting; return true;
                    }
                    if (it->second == Named::Connecting) {
                        it->second = Named::Connected; return true;
                    }
                    return true;
                },
                [&](trait::Connections::read, std::string) -> std::string { return "data"; });
        }
    }
};

struct IffState
{
    P p = P::Off; Q q = Q::Low;
    std::map<std::string, R> r;
    std::map<std::string, S> s;
};

template<class G>
void setupIffStateMocks(G& g, IffState& st)
{
    g.mocks->defineConst(
        [&](protocol::IffProto::protoP) -> P { return st.p; },
        [&](protocol::IffProto::protoQ) -> Q { return st.q; },
        [&](protocol::IffProto::protoR, std::string k) -> R {
            auto it = st.r.find(k); return it != st.r.end() ? it->second : R::Dim;
        },
        [&](protocol::IffProto::protoS, std::string k) -> S {
            auto it = st.s.find(k); return it != st.s.end() ? it->second : S::Cold;
        });
}

void setupIffMocks(IffGraph& g, IffState& st)
{
    setupIffStateMocks(g, st);
    g.mocks->define(
        [&](trait::IffTrait::setOn)   { st.p = P::On;  },
        [&](trait::IffTrait::setOff)  { st.p = P::Off; },
        [&](trait::IffTrait::setHigh) { st.q = Q::High;},
        [&](trait::IffTrait::setLow)  { st.q = Q::Low; },
        [&](trait::IffTrait::setLit,  std::string k) { st.r[k] = R::Lit;  },
        [&](trait::IffTrait::setDim,  std::string k) { st.r[k] = R::Dim;  },
        [&](trait::IffTrait::setHot,  std::string k) { st.s[k] = S::Hot;  },
        [&](trait::IffTrait::setCold, std::string k) { st.s[k] = S::Cold; });
}

void setupIffProtoMocks(IffProtoGraph& g, IffState& st)
{
    setupIffStateMocks(g, st);
    g.mocks->defineConst(
        [&](protocol::IffProto::isOn) -> bool { return st.p == P::On; },
        [&](protocol::IffProto::isLit, std::string k) -> bool {
            auto it = st.r.find(k); return it != st.r.end() && it->second == R::Lit;
        });
}

// ===========================================================================
// Transition table
// ===========================================================================

TEST_CASE("transition table — declared transitions and self-transitions are valid; others are not")
{
    constexpr struct { Main f, t; bool ok; } nullary[] = {
        // declared
        {Main::Closed, Main::Open,   true},  {Main::Open,   Main::Closed, true},
        {Main::Open,   Main::Error,  true},  {Main::Error,  Main::Closed, true},
        {Main::Error,  Main::Dead,   true},
        // undeclared
        {Main::Closed, Main::Error,  false}, {Main::Closed, Main::Dead,   false},
        {Main::Open,   Main::Dead,   false}, {Main::Error,  Main::Open,   false},
        {Main::Dead,   Main::Closed, false}, {Main::Dead,   Main::Open,   false},
        {Main::Dead,   Main::Error,  false},
        // self-transitions
        {Main::Closed, Main::Closed, true},  {Main::Open,   Main::Open,   true},
        {Main::Error,  Main::Error,  true},  {Main::Dead,   Main::Dead,   true},
    };
    for (auto [f, t, ok] : nullary) {
        CAPTURE(toString(f)); CAPTURE(toString(t));
        CHECK(isValidTransition(f, t) == ok);
    }

    constexpr struct { Named f, t; bool ok; } keyed[] = {
        // declared (set form `Disconnected <-- Connecting | Connected` expands here)
        {Named::Disconnected, Named::Connecting,   true},
        {Named::Connecting,   Named::Connected,    true},
        {Named::Connecting,   Named::Disconnected, true},
        {Named::Connected,    Named::Disconnected, true},
        // undeclared
        {Named::Connected,    Named::Connecting,   false},
        {Named::Disconnected, Named::Connected,    false},
        // self-transitions
        {Named::Disconnected, Named::Disconnected, true},
        {Named::Connecting,   Named::Connecting,   true},
        {Named::Connected,    Named::Connected,    true},
    };
    for (auto [f, t, ok] : keyed) {
        CAPTURE(toString(f)); CAPTURE(toString(t));
        CHECK(isValidTransition(f, t) == ok);
    }
}

// ===========================================================================
// is clause — pre-state strict enforcement
// ===========================================================================

TEST_CASE("is clause — pre-state must hold at entry for every method that requires it")
{
    constexpr Main all[] = {Main::Closed, Main::Open, Main::Error, Main::Dead};

    SUBCASE("open requires Closed") {
        for (auto s : all) {
            CAPTURE(toString(s));
            Graph g; MockState st(g); st.nullary = s;
            if (s == Main::Closed) CHECK_NOTHROW(g.node->open());
            else CHECK_THROWS_WITH_AS(g.node->open(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
        }
    }
    SUBCASE("close requires Open") {
        for (auto s : all) {
            CAPTURE(toString(s));
            Graph g; MockState st(g); st.nullary = s;
            if (s == Main::Open) CHECK_NOTHROW(g.node->close());
            else CHECK_THROWS_WITH_AS(g.node->close(), doctest::Contains("proto.Open()"), arc::ContractViolation);
        }
    }
    SUBCASE("forceOpen requires Closed") {
        for (auto s : all) {
            CAPTURE(toString(s));
            Graph g; MockState st(g); st.nullary = s;
            if (s == Main::Closed) CHECK_NOTHROW(g.node->forceOpen());
            else CHECK_THROWS_WITH_AS(g.node->forceOpen(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
        }
    }
    SUBCASE("kill requires Error") {
        for (auto s : all) {
            CAPTURE(toString(s));
            Graph g; MockState st(g); st.nullary = s;
            if (s == Main::Error) CHECK_NOTHROW(g.node->kill());
            else CHECK_THROWS_WITH_AS(g.node->kill(), doctest::Contains("proto.Error()"), arc::ContractViolation);
        }
    }
}

// ===========================================================================
// then clause
// ===========================================================================

TEST_CASE("then clause — post-condition checked against named return value (open: success == proto.Open())")
{
    Graph g; MockState st(g);

    SUBCASE("returns true and ends Open — passes") {
        CHECK_NOTHROW(g.node->open());
    }
    SUBCASE("returns false and stays Closed — passes (false == false)") {
        g.mocks->define([&](trait::Connections::open) -> bool { return false; });
        CHECK_NOTHROW(g.node->open());
    }
    SUBCASE("returns true but stays Closed — fails (true != false)") {
        g.mocks->define([&](trait::Connections::open) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->open(), doctest::Contains("proto.Open()"), arc::ContractViolation);
    }
    SUBCASE("returns true and ends Error — fails (true != false)") {
        g.mocks->define([&](trait::Connections::open) -> bool { st.nullary = Main::Error; return true; });
        CHECK_THROWS_WITH_AS(g.node->open(), doctest::Contains("proto.Open()"), arc::ContractViolation);
    }

    // Symmetric coverage on close: is Open then (success: success == proto.Closed())
    SUBCASE("close: returns false stays Open — passes (false == false)") {
        st.nullary = Main::Open;
        g.mocks->define([&](trait::Connections::close) -> bool { return false; });
        CHECK_NOTHROW(g.node->close());
    }
    SUBCASE("close: returns true but stays Open — fails") {
        st.nullary = Main::Open;
        g.mocks->define([&](trait::Connections::close) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->close(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
    }
}

TEST_CASE("then clause — bare-name post-state on void method (forceOpen: is Closed then Open)")
{
    Graph g; MockState st(g);

    SUBCASE("default mock transitions Closed→Open — passes") {
        CHECK_NOTHROW(g.node->forceOpen());
    }
    SUBCASE("does not transition — fails then(Open)") {
        g.mocks->define([&](trait::Connections::forceOpen) { /* leave nullary as Closed */ });
        CHECK_THROWS_WITH_AS(g.node->forceOpen(), doctest::Contains("proto.Open()"), arc::ContractViolation);
    }
}

// ===========================================================================
// to clause
// ===========================================================================

TEST_CASE("to clause — reset() must end Closed via a declared transition from any starting state")
{
    constexpr struct { Main from; bool valid; } cases[] = {
        {Main::Closed, true},   // self-transition
        {Main::Open,   true},   // declared
        {Main::Error,  true},   // declared
        {Main::Dead,   false},  // undeclared transition Dead→Closed
    };
    for (auto [from, valid] : cases) {
        CAPTURE(toString(from));
        Graph g; MockState st(g); st.nullary = from;
        if (valid) CHECK_NOTHROW(g.node->reset());
        else CHECK_THROWS_WITH_AS(g.node->reset(), doctest::Contains("protocol transition"), arc::ContractViolation);
    }
}

TEST_CASE("to clause — reset() must end Closed regardless of where it lands")
{
    Graph g; MockState st(g);
    st.nullary = Main::Open;

    SUBCASE("ends Error — fails to(Closed)") {
        g.mocks->define([&](trait::Connections::reset) -> bool { st.nullary = Main::Error; return true; });
        CHECK_THROWS_WITH_AS(g.node->reset(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
    }
    SUBCASE("ends Dead — fails to(Closed)") {
        g.mocks->define([&](trait::Connections::reset) -> bool { st.nullary = Main::Dead; return true; });
        CHECK_THROWS_WITH_AS(g.node->reset(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
    }
    SUBCASE("does not transition — fails to(Closed)") {
        g.mocks->define([&](trait::Connections::reset) -> bool { return false; });
        CHECK_THROWS_WITH_AS(g.node->reset(), doctest::Contains("proto.Closed()"), arc::ContractViolation);
    }
}

// ===========================================================================
// if...then clause
// ===========================================================================

TEST_CASE("if...then — connect()'s two ifs target only the matching entry condition")
{
    Graph g; MockState st(g);
    st.nullary = Main::Open;

    SUBCASE("from Disconnected, returns true → Connecting passes") {
        CHECK_NOTHROW(g.node->connect("x"));
    }
    SUBCASE("from Connecting, returns true → Connected passes") {
        st.keyed["x"] = Named::Connecting;
        CHECK_NOTHROW(g.node->connect("x"));
    }
    SUBCASE("from Connected, both ifs false → no exit constraint, no-op passes") {
        st.keyed["x"] = Named::Connected;
        CHECK_NOTHROW(g.node->connect("x"));
    }
    SUBCASE("from Disconnected, returns false but stays Connecting — violates r ? Connecting : Disconnected") {
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.keyed[n] = Named::Connecting; return false;
        });
        CHECK_THROWS_WITH_AS(g.node->connect("x"), doctest::Contains("proto.Connecting(name)"), arc::ContractViolation);
    }
    SUBCASE("from Disconnected, returns true but jumps to Connected — violates r ? Connecting") {
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.keyed[n] = Named::Connected; return true;
        });
        CHECK_THROWS_WITH_AS(g.node->connect("x"), doctest::Contains("proto.Connecting(name)"), arc::ContractViolation);
    }
    SUBCASE("from Disconnected, returns true but stays Disconnected — violates r ? Connecting") {
        g.mocks->define([&](trait::Connections::connect, std::string) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->connect("x"), doctest::Contains("proto.Connecting(name)"), arc::ContractViolation);
    }
}

// ===========================================================================
// No exit clause for a group → that group must not change
// ===========================================================================

TEST_CASE("methods without to/then for a group must keep that group's state identical")
{
    Graph g; MockState st(g);
    st.nullary = Main::Open;

    SUBCASE("connect() may not mutate nullary (no to/then on Main group)") {
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.nullary = Main::Error;          // unannounced nullary change
            st.keyed[n] = Named::Connecting;
            return true;
        });
        CHECK_THROWS_AS(g.node->connect("x"), arc::ContractViolation);
    }
    SUBCASE("connect() may not mutate keyed when both ifs are false") {
        st.keyed["x"] = Named::Connected;  // both ifs false for x
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.keyed[n] = Named::Disconnected; return true;
        });
        CHECK_THROWS_AS(g.node->connect("x"), arc::ContractViolation);
    }
    SUBCASE("read() — has only an `is` clause; nullary mutation throws") {
        st.keyed["x"] = Named::Connected;
        g.mocks->define([&](trait::Connections::read, std::string) -> std::string {
            st.nullary = Main::Closed; return "data";
        });
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("Open -> Closed"), arc::ContractViolation);
    }
    SUBCASE("read() — keyed mutation also throws") {
        st.keyed["x"] = Named::Connected;
        g.mocks->define([&](trait::Connections::read, std::string n) -> std::string {
            st.keyed[n] = Named::Disconnected; return "data";
        });
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("Connected -> Disconnected"), arc::ContractViolation);
    }
    SUBCASE("forceOpen() — has then(Open) for Main only; nullary unchanged fails then(Open)") {
        st.nullary = Main::Closed;
        g.mocks->define([&](trait::Connections::forceOpen) {
            st.keyed["x"] = Named::Connecting;  // unannounced keyed mutation; nullary unchanged
        });
        CHECK_THROWS_WITH_AS(g.node->forceOpen(), doctest::Contains("proto.Open()"), arc::ContractViolation);
    }
}

// ===========================================================================
// is predicate(args) — predicate as a pre-condition
// ===========================================================================

TEST_CASE("is predicate(args) — read() requires hasPendingData(name) on entry")
{
    Graph g; MockState st(g);
    st.nullary = Main::Open;

    SUBCASE("Connected key — hasPendingData true, passes") {
        st.keyed["x"] = Named::Connected;
        CHECK_NOTHROW(g.node->read("x"));
    }
    SUBCASE("Disconnected key — hasPendingData false, throws") {
        st.keyed["x"] = Named::Disconnected;
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("hasPendingData"), arc::ContractViolation);
    }
    SUBCASE("Connecting key — hasPendingData false, throws") {
        st.keyed["x"] = Named::Connecting;
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("hasPendingData"), arc::ContractViolation);
    }
    SUBCASE("Unknown key — hasPendingData false, throws") {
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("hasPendingData"), arc::ContractViolation);
    }
}

// ===========================================================================
// implies invariant — Connected implies Open
// ===========================================================================

TEST_CASE("implies invariant — Connected implies Open enforced at entry and exit")
{
    SUBCASE("entry: read() with hasPendingData precondition transitively checks implies — Connected key with Closed nullary throws") {
        Graph g; MockState st(g);
        st.nullary = Main::Open;
        st.keyed["x"] = Named::Connected;
        // After close(), nullary=Closed but keyed still Connected — violates implies
        g.mocks->define([&](trait::Connections::close) -> bool {
            st.nullary = Main::Closed;
            return true;
        });
        CHECK_NOTHROW(g.node->close());
        // Now nullary=Closed, keyed["x"]=Connected. read("x") passes hasPendingData since Connected("x")=true,
        // but the Connected implies Open entry invariant should catch the inconsistency.
        CHECK_THROWS_WITH_AS(g.node->read("x"), doctest::Contains("implies"), arc::ContractViolation);
    }
    SUBCASE("exit: connect() with unannounced nullary change throws transition violation first") {
        Graph g; MockState st(g);
        st.nullary = Main::Open;
        st.keyed["x"] = Named::Connecting;
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.keyed[n] = Named::Connected;
            st.nullary = Main::Closed;
            return true;
        });
        CHECK_THROWS_WITH_AS(g.node->connect("x"), doctest::Contains("unannounced transition"), arc::ContractViolation);
    }
    SUBCASE("exit: connect() ends Connected with nullary still Open — passes") {
        Graph g; MockState st(g);
        st.nullary = Main::Open;
        st.keyed["x"] = Named::Connecting;
        g.mocks->define([&](trait::Connections::connect, std::string n) -> bool {
            st.keyed[n] = Named::Connected;
            return true;
        });
        CHECK_NOTHROW(g.node->connect("x"));
    }
    SUBCASE("invariants only checked for keys touched by this call (per-key isolation)") {
        Graph g; MockState st(g);
        st.nullary = Main::Open;
        st.keyed["y"] = Named::Connected;     // y consistent with Open
        // close() transitions Open→Closed without touching keyed; the existing Connected("y")
        // would violate `Connected implies Open` if checked, but per-key isolation skips
        // untouched keys.
        CHECK_NOTHROW(g.node->close());
    }
}

// ===========================================================================
// iff invariant — On iff High (nullary) and Lit iff Hot (keyed)
// ===========================================================================

TEST_CASE("iff invariant nullary — On iff High caught from each side, entry and exit")
{
    IffGraph g; IffState st; setupIffMocks(g, st);

    SUBCASE("entry: starts (On, Low) — setOn throws on entry check") {
        st.p = P::On; st.q = Q::Low;
        CHECK_THROWS_WITH_AS(g.node->setOn(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("entry: starts (Off, High) — setHigh throws on entry check") {
        st.p = P::Off; st.q = Q::High;
        CHECK_THROWS_WITH_AS(g.node->setHigh(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setOn from (Off, Low) → (On, Low) violates iff (source-side flips on)") {
        CHECK_THROWS_WITH_AS(g.node->setOn(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setOff from (On, High) → (Off, High) violates iff (source-side flips off)") {
        st.p = P::On; st.q = Q::High;
        CHECK_THROWS_WITH_AS(g.node->setOff(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setHigh from (Off, Low) → (Off, High) violates iff (target-side flips on)") {
        CHECK_THROWS_WITH_AS(g.node->setHigh(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setLow from (On, High) → (On, Low) violates iff (target-side flips off)") {
        st.p = P::On; st.q = Q::High;
        CHECK_THROWS_WITH_AS(g.node->setLow(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("self-transitions in valid combined states pass") {
        CHECK_NOTHROW(g.node->setOff());   // (Off, Low) — both sides false
        CHECK_NOTHROW(g.node->setLow());
        st.p = P::On; st.q = Q::High;
        CHECK_NOTHROW(g.node->setOn());    // (On, High) — both sides true
        CHECK_NOTHROW(g.node->setHigh());
    }
}

TEST_CASE("iff invariant keyed — Lit iff Hot caught from each side, entry and exit")
{
    IffGraph g; IffState st; setupIffMocks(g, st);

    SUBCASE("exit: setLit on default (Dim, Cold) → (Lit, Cold) violates iff") {
        CHECK_THROWS_WITH_AS(g.node->setLit("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setHot on default (Dim, Cold) → (Dim, Hot) violates iff") {
        CHECK_THROWS_WITH_AS(g.node->setHot("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setDim from (Lit, Hot) violates iff (source flips off, target stays)") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;
        CHECK_THROWS_WITH_AS(g.node->setDim("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("exit: setCold from (Lit, Hot) violates iff (target flips off, source stays)") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;
        CHECK_THROWS_WITH_AS(g.node->setCold("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("self-transitions on valid keys pass") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;
        CHECK_NOTHROW(g.node->setLit("x"));
        CHECK_NOTHROW(g.node->setHot("x"));
        // default key "y" is (Dim, Cold) — also valid
        CHECK_NOTHROW(g.node->setDim("y"));
        CHECK_NOTHROW(g.node->setCold("y"));
    }
    SUBCASE("invariants only checked for keys touched by this call (per-key isolation)") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;     // x valid
        st.s["y"] = S::Hot;                          // y=(Dim, Hot) would violate iff if checked
        CHECK_NOTHROW(g.node->setLit("x"));
    }
}

// ===========================================================================
// Predicate methods — implies / iff
// ===========================================================================

TEST_CASE("implies predicate — hasPendingData implies Connected(name)")
{
    ProtoGraph g; MockState st(g);

    SUBCASE("returns false — implies trivially holds") {
        CHECK_NOTHROW(g.node->hasData("x"));
    }
    SUBCASE("returns true while Connected — passes") {
        st.nullary = Main::Open;
        st.keyed["x"] = Named::Connected;
        g.mocks->defineConst([&](protocol::Proto::hasPendingData, std::string) -> bool { return true; });
        CHECK_NOTHROW(g.node->hasData("x"));
    }
    SUBCASE("returns true while Disconnected — implies violated, throws") {
        st.keyed["x"] = Named::Disconnected;
        g.mocks->defineConst([&](protocol::Proto::hasPendingData, std::string) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->hasData("x"), doctest::Contains("implies"), arc::ContractViolation);
    }
}

TEST_CASE("iff predicate nullary — isOn iff On checked in both directions")
{
    IffProtoGraph g; IffState st; setupIffProtoMocks(g, st);

    SUBCASE("isOn() false while Off — passes (default mock)") {
        CHECK_NOTHROW(g.node->isOn());
    }
    SUBCASE("isOn() true while On — passes") {
        st.p = P::On; st.q = Q::High;
        g.mocks->defineConst([&](protocol::IffProto::isOn) -> bool { return true; });
        CHECK_NOTHROW(g.node->isOn());
    }
    SUBCASE("isOn() returns true while Off — throws") {
        g.mocks->defineConst([&](protocol::IffProto::isOn) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->isOn(), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("isOn() returns false while On — throws") {
        st.p = P::On; st.q = Q::High;
        g.mocks->defineConst([&](protocol::IffProto::isOn) -> bool { return false; });
        CHECK_THROWS_WITH_AS(g.node->isOn(), doctest::Contains("iff"), arc::ContractViolation);
    }
}

TEST_CASE("iff predicate keyed — isLit(k) iff Lit(k) checked in both directions")
{
    IffProtoGraph g; IffState st; setupIffProtoMocks(g, st);

    SUBCASE("isLit(x) false, key Dim — passes (default mock)") {
        CHECK_NOTHROW(g.node->isLit("x"));
    }
    SUBCASE("isLit(x) true, key Lit — passes") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;
        g.mocks->defineConst([&](protocol::IffProto::isLit, std::string) -> bool { return true; });
        CHECK_NOTHROW(g.node->isLit("x"));
    }
    SUBCASE("isLit(x) returns true while Dim — throws") {
        g.mocks->defineConst([&](protocol::IffProto::isLit, std::string) -> bool { return true; });
        CHECK_THROWS_WITH_AS(g.node->isLit("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
    SUBCASE("isLit(x) returns false while Lit — throws") {
        st.r["x"] = R::Lit; st.s["x"] = S::Hot;
        g.mocks->defineConst([&](protocol::IffProto::isLit, std::string) -> bool { return false; });
        CHECK_THROWS_WITH_AS(g.node->isLit("x"), doctest::Contains("iff"), arc::ContractViolation);
    }
}

// ===========================================================================
// Multi-step sequences
// ===========================================================================

TEST_CASE("multi-step sequences — declared chains work; recovery via reset; kill from Error")
{
    Graph g; MockState st(g);

    SUBCASE("open → close round-trip") {
        CHECK_NOTHROW(g.node->open());
        CHECK_NOTHROW(g.node->close());
        CHECK_NOTHROW(g.node->open());
        CHECK_NOTHROW(g.node->close());
    }
    SUBCASE("multiple independent keys via connect") {
        st.nullary = Main::Open;
        CHECK_NOTHROW(g.node->connect("a"));
        CHECK_NOTHROW(g.node->connect("b"));
        CHECK_NOTHROW(g.node->connect(""));
    }
    SUBCASE("error recovery: Error → reset → Closed") {
        st.nullary = Main::Error;
        CHECK_NOTHROW(g.node->reset());
    }
    SUBCASE("open → fault to Error → kill → Dead") {
        CHECK_NOTHROW(g.node->open());
        st.nullary = Main::Error;
        CHECK_NOTHROW(g.node->kill());
    }
}

// ===========================================================================
// Protocol view — asTrait(arc::protocol(trait)) reads state without checks
// ===========================================================================

TEST_CASE("protocol view — variant queries through asTrait(arc::protocol(...)) do not trigger checks")
{
    Graph g; MockState st(g);
    auto proto = g.mocks->asTrait(arc::protocol(trait::connections));

    SUBCASE("nullary variant query after open") {
        CHECK_NOTHROW(g.node->open());
        CHECK(proto.Open());
        CHECK_FALSE(proto.Closed());
    }
    SUBCASE("parameterised variant query after connect") {
        st.nullary = Main::Open;
        CHECK_NOTHROW(g.node->connect("x"));
        CHECK(proto.Connecting("x"));
        CHECK_NOTHROW(g.node->connect("x"));
        CHECK(proto.Connected("x"));
    }
    SUBCASE("predicate query through view") {
        CHECK_FALSE(proto.isDead());  // default state is Closed
    }
    SUBCASE("parameterised predicate query through view") {
        st.nullary = Main::Open;
        st.keyed["x"] = Named::Connected;
        CHECK(proto.hasPendingData("x"));
        CHECK_FALSE(proto.hasPendingData("unknown"));
    }
    SUBCASE("raw enum accessors through view") {
        CHECK(toString(proto.protoMain()) == toString(Main::Closed));
        CHECK(toString(proto.protoNamed("nonexistent")) == toString(Named::Disconnected));
    }
}

} // namespace arc::tests::protocol_test
