#ifndef INCLUDE_ARC_TRAITS_DATASTORE_HPP
#define INCLUDE_ARC_TRAITS_DATASTORE_HPP

#include "arc/macros.hpp"

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

private:
    T const* data;
    std::size_t eventId;
    std::size_t const* eventIdPtr;
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
