#include "arc/detail/concepts.hpp"

namespace arc {

// Implemented by arc::DataStoreNode<SharedData> aka arc::node::DataStore<SharedData>
template<class SharedData>
trait DataStore [Types]
{
    requires typename SharedData::Id

    // PrivateData is defined by the calling node if it is a DataListener.
    // When calling from a non-listener, PrivateData = void.
    type PrivateData

    // add: try to emplace a new SharedData with for the given id with the constructor args.
    // Returns a pair of (pointer to SharedData, whether it was subscribed to by any new listeners).
    // If no listeners subscribe to the SharedData, it is not added and nullptr is returned.
    // If called again with the same id, no new SharedData is created, and the existing one is returned.
    // Any listeners that were not previously subscribed to the id are called with DataListener::init again.
    add(SharedData::Id const& id, auto&&... sharedDataArgs) -> std::pair<SharedData const*, bool>

    // get: returns pointers to SharedData and PrivateData with the id if it exists.
    get(SharedData::Id const& id) -> std::pair<SharedData const*, typename Types::PrivateData*>
    get(SharedData::Id const& id) const -> std::pair<SharedData const*, typename Types::PrivateData const*>
    // get: by SharedData pointer
    get(SharedData const* sharedData) -> Types::PrivateData*
    get(SharedData const* sharedData) const -> Types::PrivateData const*

    // modify: modify the SharedData and any PrivateData with the given id using the provided function.
    // The function should take (SharedData [const]*, PrivateData*) and return an Event to notify listeners with.
    // The caller is not notified; the returned Event is used to notify the other listeners after modification.
    // If Event is void or arc::noEvent, no notification is sent, and SharedData const* is supplied to useData.
    // If Event is std::optional<T>, notification is only sent if the optional has a value.
    // Else, notification is always sent.
    // Returns const pointers to SharedData and PrivateData after any resulting event has been processed by the listeners.
    template<std::invocable<SharedData*, typename Types::PrivateData*> UseData>
    modify(SharedData::Id const& id, UseData&& useData) -> std::pair<SharedData const*, typename Types::PrivateData*>
    // modify: by SharedData pointer
    template<std::invocable<SharedData*, typename Types::PrivateData*> UseData>
    modify(SharedData const* sharedData, UseData&& useData) -> Types::PrivateData*

    // forEach: access the SharedData and any PrivateData for all ids using the provided function.
    // The function should take (SharedData [const]*, PrivateData [const]*)
    // Mutable SharedData* is only available when a non-void Event is returned to notify listeners with.
    // The caller is not notified; the returned Event is used to notify the other listeners after modification.
    // Returns const pointers to SharedData and PrivateData after any resulting event has been processed by the listeners.
    template<std::invocable<typename SharedData::Id const&, SharedData*, typename Types::PrivateData*> UseData>
    forEach(UseData&& useData) -> void
    template<std::invocable<typename SharedData::Id const&, SharedData const*, typename Types::PrivateData const*> UseData>
    forEach(UseData&& useData) const -> void

    // notify: notify listeners of an event for the given id.
    // If id exists, returns const pointers to SharedData and PrivateData after the event has been processed by the listeners.
    notify(auto const& event, SharedData::Id const& id) -> std::pair<SharedData const*, typename Types::PrivateData*>
    // notify: by SharedData pointer
    notify(auto const& event, SharedData const* sharedData) -> Types::PrivateData*

    // notifyAll: notify all listeners of an event for all ids.
    notifyAll(auto const& event) -> void
}

// Implemented by nodes listening to arc::DataStoreNode<SharedData>
template<class SharedData>
trait DataListener [Types]
{
    requires typename SharedData::Id

    // PrivateData for this listener.
    // Set to arc::NoPrivateData if no private data is needed.
    type PrivateData
    requires !std::is_void_v<typename Types::PrivateData>

    // init: initialise PrivateData for the given id to subscribe to events on this id.
    // If the listener conditionally subscribes to a subset of ids, the return type should be void,
    //    and the provided constructor should only be called for the ids that it subscribes to.
    // If the constructor is not called, the listener will not be subscribed to the id, and no PrivateData is created for it.
    // privateDataConstructor: (Args&&...) -> PrivateData*
    // To unconditionally subscribe to all ids, this should simply return PrivateData directly instead of calling the constructor.
    // Unconditionally subscribing to all ids saves a branch in the notify path.
    init(SharedData::Id const& id, SharedData& sharedData, auto privateDataConstructor) -> arc::detail::SameAsOrVoid<typename Types::PrivateData> auto

    // skipEvent: return true to skip processing the event for this listener.
    // Called on each subscriber for the id when modify, notify, or notifyAll is called on the DataStore, before onEvent is called.
    // A useful catch-all implementation might be: `return not event.isLatest();`
    // If not defined for an event, the default is "do not skip".
    template<class EventType>
    skipEvent(Event<EventType> event, EventItem<SharedData const, typename Types::PrivateData const> item) -> std::convertible_to<bool> auto
    {
        return std::false_type{};
    }

    // Called on all subscribers when modify, notify, or notifyAll is called on the DataStore.
    // If the return type is `bool` and `false` is returned, then this particular event is not propagated to any further listeners.
    template<class EventType>
    onEvent(Event<EventType> event, EventItem<SharedData, typename Types::PrivateData> item) -> arc::detail::SameAsOrVoid<bool> auto
}

} // namespace arc
