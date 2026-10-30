#include "arc/tests/test_nodiscard.hxx"

#include "arc/arc.hpp"

#include <doctest/doctest.h>

// Compiled once as a test, and again with each ARC_DISCARD_* case defined, where the discarded result must fail the
// compile: as a discarded call, or, through a fan-out, as a call whose results ARC itself would drop.

#if 0
// arc-begin
#include "arc/arc.hpp"

trait arc::tests::nodiscard::trait::Answer
{
    [[nodiscard]] answer() const -> int
    [[nodiscard]] scaled(int factor) const -> int
    [[nodiscard]] scaled(int factor, int offset) const -> int
    template<class T>
    [[nodiscard]] ! echo(T value) const -> T
    [[nodiscard]] doubled() const -> int { return self.answer() * 2; }
    [[nodiscard]] tripled() const -> int
    {
#if defined(ARC_DISCARD_SELF)
        self.answer();
#endif
        return self.answer() * 3;
    }
    note(int value)
    noted() const -> int
}
// arc-end

// arc-begin
cluster arc::tests::nodiscard::AnswerCluster [R = Root]
{
    answerer = R::Answerer

    [trait::Answer]
    .. --> answerer
}
// arc-end

// arc-begin
// Viewable through a repeater: the only [[nodiscard]] method is optional, so making the view asserts nothing of it
trait arc::tests::nodiscard::trait::Tally
{
    [[nodiscard]] ? tally() const -> int
    count()
}
// arc-end

// arc-begin
cluster arc::tests::nodiscard::TallyCluster [R = Root]
{
    first = R::Tallier
    second = R::Tallier

    [trait::Tally]
    .. --> {first, second}
}
// arc-end

// arc-begin
cluster arc::tests::nodiscard::AnswerersCluster [R = Root]
{
    answerers = R::Answerer : arc::StaticMap<int>

    [trait::Answer]
    .. --> answerers
}
// arc-end

#endif

namespace arc::tests::nodiscard {

struct Answerer : arc::NodeImpl<trait::Answer>
{
    int answer() const { return 42; }
    int scaled(int factor) const { return 42 * factor; }
    int scaled(int factor, int offset) const { return 42 * factor + offset; }
    template<class T>
    T echo(T value) const { return value; }
    void note(int value) { last = value; }
    int noted() const { return last; }

    int last = 0;
};

// Implements by tag rather than through arc::NodeImpl, so a defaulted body it leaves alone runs against the view
struct TagAnswerer : arc::Node
{
    using Traits = arc::Traits<trait::Answer>;

    int impl(trait::Answer::answer) const { return 42; }
    int impl(trait::Answer::scaled, int factor) const { return 42 * factor; }
    int impl(trait::Answer::scaled, int factor, int offset) const { return 42 * factor + offset; }
    template<class T>
    T impl(trait::Answer::echo, T value) const { return value; }
    void impl(trait::Answer::note, int value) { last = value; }
    int impl(trait::Answer::noted) const { return last; }

    int last = 0;
};

struct Tallier : arc::NodeImpl<trait::Tally>
{
    int tally() const { return counted; }
    void count() { ++counted; }

    int counted = 0;
};

struct Root
{
    using Answerer = nodiscard::Answerer;
    using Tallier = nodiscard::Tallier;
};

struct TagRoot
{
    using Answerer = TagAnswerer;
};

// Views the trait through its duck methods, as arc::key::ThreadPost does
struct DuckKey : arc::key::Default
{
    template<class T>
    using Trait = arc::detail::DuckTrait<T>;
};

static_assert(arc::IsNoDiscardMethod<trait::Answer::answer>);
static_assert(arc::IsNoDiscardMethod<trait::Answer::scaled>);
static_assert(arc::IsNoDiscardMethod<trait::Answer::echo>);
static_assert(arc::IsNoDiscardMethod<trait::Answer::doubled>);
static_assert(arc::IsNoDiscardMethod<trait::Answer::tripled>);
static_assert(not arc::IsNoDiscardMethod<trait::Answer::note>);
static_assert(not arc::IsNoDiscardMethod<trait::Answer::noted>);

} // namespace arc::tests::nodiscard

using namespace arc::tests::nodiscard;

TEST_CASE("nodiscard trait methods answer through every call surface")
{
    arc::Graph<cluster::AnswerCluster, Root> graph{};
    auto answer = graph.asTrait(trait::answer);

    CHECK(answer.answer() == 42);
    CHECK(answer.scaled(2) == 84);
    CHECK(answer.scaled(2, 1) == 85);
    CHECK(answer.echo(7) == 7);
    CHECK(answer.doubled() == 84);
    CHECK(answer.tripled() == 126);
    CHECK(answer.impl(trait::Answer::answer_c) == 42);
    CHECK(answer.answer(arc::asFunctor)() == 42);

    arc::Graph<cluster::AnswerCluster, TagRoot> byTag{};
    CHECK(byTag.asTrait(trait::answer).tripled() == 126);

    arc::Graph<cluster::AnswerersCluster, Root> answerers{.answerers{2, [](auto add) { add(0); add(1); }}};
    auto keyed = answerers.asTrait(trait::answer, arc::key::Element(1));
    CHECK(keyed.answer() == 42);
    auto ducked = graph.asTrait(trait::answer, DuckKey{});
    CHECK(ducked.answer() == 42);

    // A fan-out keeps no target's result, so it takes only methods whose result may be dropped
    auto everyElement = answerers.asTrait(trait::answer, arc::key::allElements);
    everyElement.note(3);
    arc::Graph<cluster::TallyCluster, Root> tallies{};
    auto everyTally = tallies.asTrait(trait::tally);
    everyTally.count();
    CHECK(tallies.first->tally() == 1);

    // Methods not declared [[nodiscard]] stay discardable, whatever they return
    answer.note(5);
    answer.noted();
    answer.impl(trait::Answer::noted_c);
    answer.noted(arc::asFunctor)();
    keyed.noted();
    CHECK(answer.noted() == 5);

#if defined(ARC_DISCARD_NAMED)
    answer.answer();
#elif defined(ARC_DISCARD_OVERLOAD)
    answer.scaled(2, 1);
#elif defined(ARC_DISCARD_EXPLICIT)
    answer.echo(7);
#elif defined(ARC_DISCARD_DEFAULT)
    answer.doubled();
#elif defined(ARC_DISCARD_TAG)
    answer.impl(trait::Answer::answer_c);
#elif defined(ARC_DISCARD_FUNCTOR)
    answer.answer(arc::asFunctor)();
#elif defined(ARC_DISCARD_KEYED)
    keyed.answer();
#elif defined(ARC_DISCARD_DUCK)
    ducked.answer();
#elif defined(ARC_DISCARD_FANOUT)
    everyElement.answer();
#elif defined(ARC_DISCARD_REPEATER)
    everyTally.impl(trait::Tally::tally_c);
#endif
    // ARC_DISCARD_SELF discards inside tripled()'s default body, which byTag runs against its view
}
