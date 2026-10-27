#ifndef INCLUDE_ARC_TRAITS_DATASTORE_HPP
#define INCLUDE_ARC_TRAITS_DATASTORE_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct EventId
{
    constexpr explicit EventId(std::size_t id) : id(id) {}

    auto operator<=>(EventId const&) const = default;

private:
    std::size_t id;
};

ARC_MODULE_EXPORT
template<class T>
struct Event
{
    constexpr explicit Event(T const* data, std::size_t const* eventIdPtr)
        : data(data)
        , eventId(*eventIdPtr)
        , eventIdPtr(eventIdPtr)
    {}

    constexpr EventId getEventId() const { return EventId(eventId); }
    constexpr bool isLatest() const { return eventId == *eventIdPtr; }

    constexpr T const* operator->() const { return data; }
    constexpr T const& operator*() const { return *data; }
    constexpr T const& get() const { return *data; }

private:
    T const* data;
    std::size_t eventId;
    std::size_t const* eventIdPtr;
};

// The only persistable reference to a DataStore entry. It does not know its store.
ARC_MODULE_EXPORT
template<class SharedData>
class DataHandle
{
public:
    static constexpr std::uint32_t noSlot = std::numeric_limits<std::uint32_t>::max();

    constexpr explicit DataHandle(std::uint32_t slot, std::uint32_t generation) noexcept
        : slotIndex(slot), slotGeneration(generation)
    {}

    constexpr std::uint32_t slot() const noexcept { return slotIndex; }
    constexpr std::uint32_t generation() const noexcept { return slotGeneration; }

    // Taken from a released entry
    constexpr bool empty() const noexcept { return slotIndex == noSlot; }

    constexpr bool operator==(DataHandle const& other) const noexcept { return key() == other.key(); }
    constexpr auto operator<=>(DataHandle const& other) const noexcept { return key() <=> other.key(); }

private:
    constexpr std::uint64_t key() const noexcept { return std::uint64_t{slotIndex} << 32 | slotGeneration; }

    std::uint32_t slotIndex;
    std::uint32_t slotGeneration;
};

ARC_MODULE_EXPORT
template<class H>
concept IsDataHandle = std::copyable<H> and std::totally_ordered<H> and requires(H const& h) {
    { h.empty() } -> std::same_as<bool>;
};

ARC_MODULE_EXPORT
template<class SharedData, class PrivateData>
struct EventItem
{
    SharedData::Id const& id;
    DataHandle<std::remove_const_t<SharedData>> handle;
    SharedData& sharedData;
    PrivateData& privateData;
};

// Reads the SharedData like a pointer, but cannot be dereferenced as a temporary, so that what it reads cannot outlive
// the pin
ARC_MODULE_EXPORT
template<class R, class SharedData>
concept IsDataPointerLike =
    not std::convertible_to<R const&, bool>
    and not requires(R const& r) { *std::move(r); }
    and requires(R const& r) {
        static_cast<bool>(r);
        { *r } -> std::same_as<SharedData const&>;
        { r.operator->() } -> std::same_as<SharedData const*>;
    };

// Cannot be copied, moved or allocated with new, so that every pin is taken explicitly and stays in the scope it was
// taken in
ARC_MODULE_EXPORT
template<class R>
concept IsDataPin =
    not std::constructible_from<R, R const&> and not std::constructible_from<R, R&&>
    and not std::is_copy_assignable_v<R> and not std::is_move_assignable_v<R>
    and not requires(R (*make)()) { new auto(make()); };

ARC_MODULE_EXPORT
template<class R, class SharedData, class Handle>
concept IsDataRef =
    IsDataPin<R>
    and IsDataPointerLike<R, SharedData>
    and requires(R const& r) {
        { r.id() } -> std::same_as<typename SharedData::Id const&>;
        { r.handle() } -> std::same_as<Handle>;
        { r.clone() } -> std::same_as<R>;
    };

// A Ref paired with the caller's PrivateData, which is void for a caller that is not a DataListener
ARC_MODULE_EXPORT
template<class P, class SharedData, class Ref, class PrivateData>
concept IsDataPrivateRef =
    IsDataPin<P>
    and IsDataPointerLike<P, SharedData>
    and std::same_as<decltype(P::ref), Ref>
    and std::same_as<decltype(P::privateData), PrivateData*>
    and requires(P const& p) {
        { p.id() } -> std::same_as<typename SharedData::Id const&>;
        { p.handle() } -> std::same_as<decltype(p.ref.handle())>;
        { p.clone() } -> std::same_as<Ref>;
    };

ARC_MODULE_EXPORT
template<class A, class SharedData, class Ref, class PrivateData>
concept IsDataAdded = IsDataPrivateRef<A, SharedData, Ref, PrivateData> and std::same_as<decltype(A::inserted), bool>;

ARC_MODULE_EXPORT
template<class R, class Types>
concept IsDataRefFor = std::same_as<R, typename Types::Ref>
                    or std::same_as<R, typename Types::PrivateRef>
                    or std::same_as<R, typename Types::ConstPrivateRef>;

} // namespace arc

#include "arc/traits/datastore.hxx"

#endif // INCLUDE_ARC_TRAITS_DATASTORE_HPP
