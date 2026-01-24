#ifndef INCLUDE_ARC_TRAITS_DATASTORE_HPP
#define INCLUDE_ARC_TRAITS_DATASTORE_HPP

#include "arc/macros.hpp"

namespace arc {

ARC_MODULE_EXPORT
struct EventTracker
{
    constexpr explicit EventTracker(std::size_t const* eventIdPtr)
        : eventId(*eventIdPtr)
        , eventIdPtr(eventIdPtr)
    {}

    constexpr EventTracker getLatest() const { return EventTracker(eventIdPtr); }
    constexpr bool isLatest() const { return eventId == *eventIdPtr; }

    bool operator==(EventTracker const& other) const = default;

private:
    std::size_t eventId;
    std::size_t const* eventIdPtr;
};

ARC_MODULE_EXPORT
template<class T>
struct Event
{
    EventTracker tracker;
    T const& data;

    constexpr bool isLatest() const { return tracker.isLatest(); }

    auto* operator->(this auto& self) { return std::addressof(self.data); }
};

ARC_MODULE_EXPORT
template<class SharedData, class PrivateData>
struct EventItem
{
    SharedData::Id const& id;
    SharedData& sharedData;
    PrivateData& privateData;
};

} // namespace arc

#include "arc/traits/datastore.hxx"

#endif // INCLUDE_ARC_TRAITS_DATASTORE_HPP
