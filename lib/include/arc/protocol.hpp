#ifndef INCLUDE_ARC_PROTOCOL_HPP
#define INCLUDE_ARC_PROTOCOL_HPP

#include "arc/trait.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <tuple>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class T>
concept IsProtocol = IsTrait<T> and requires { typename T::Meta::States; };

ARC_MODULE_EXPORT
template<class T>
concept HasProtocol = IsTrait<T> and not IsProtocol<T> and requires { typename T::Meta::Protocol; };

ARC_MODULE_EXPORT
template<IsProtocol P>
using States = P::Meta::States;

ARC_MODULE_EXPORT
template<HasProtocol T>
using Protocol = T::Meta::Protocol;

ARC_MODULE_EXPORT
template<HasProtocol T>
constexpr auto protocol(T = {})
{
    return Protocol<T>{};
}

ARC_MODULE_EXPORT
template<HasProtocol T>
using ProtocolStates = T::Meta::Protocol::Meta::States;

ARC_MODULE_EXPORT
template<class GroupEnum, class... Keys>
struct ProtocolStateLog
{
    std::tuple<Keys...> key;
    GroupEnum entry;
    std::optional<GroupEnum> exit{};
};

namespace detail {
    template<IsProtocol P>
    auto getStateTransitionLog() -> P::Meta::StateTransitionLog;
    template<HasProtocol T>
    auto getStateTransitionLog() -> Protocol<T>::Meta::StateTransitionLog;
}

ARC_MODULE_EXPORT
template<IsTrait T>
using StateTransitionLog = decltype(detail::getStateTransitionLog<T>());

ARC_MODULE_EXPORT
template<class Trait, std::size_t Ifs, class... Args>
struct ProtoSnap
{
    [[no_unique_address]] std::tuple<Args...> args{};
    [[no_unique_address]] std::array<bool, Ifs> ifs{};
    StateTransitionLog<Trait> log{};
};

ARC_MODULE_EXPORT
template<class Assert, class Self, class Enum, class Proto>
ARC_INLINE constexpr void checkTransition(
    Assert& assert_, Self const& self,
    Enum entry, std::optional<Enum> const& exit,
    char const* transitionMsg, char const* unannouncedMsg, Proto const& proto, auto const&... args)
{
    if (exit.has_value())
    {
        if (not isValidTransition(entry, *exit))
            assert_(self, false, std::format("{}: {} -> {}", transitionMsg, toString(entry), toString(*exit)).c_str());
    }
    else
    {
        Enum const exit = proto.impl(args...);
        if (exit != entry)
            assert_(self, false, std::format("{}: {} -> {}", unannouncedMsg, toString(entry), toString(exit)).c_str());
    }
}

} // namespace arc

#endif // INCLUDE_ARC_PROTOCOL_HPP
