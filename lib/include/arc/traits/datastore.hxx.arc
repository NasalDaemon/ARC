#include "arc/detail/concepts.hpp"

namespace arc {

// Entries are read through a Ref. A Ref pins its entry: while the Ref is alive, nothing the store does destroys or
// moves the entry. A Ref cannot be copied, moved or allocated with new, so an object holding a Ref keeps the entry
// pinned for as long as the object lives; ref.clone() pins the entry again. A Ref is the same type for every caller,
// so any node reaching the store may be handed one, and can resolve it into its own PrivateRef.
// A PrivateRef pairs a Ref with the caller's PrivateData as it was at the lookup, and reads like its Ref: it
// dereferences, tests and has id(), handle() and clone() as the Ref does.
// A Handle is the only persistable reference to an entry.
// The modify, notify, subscribe and release overloads taking a Ref also accept the caller's own PrivateRef or
// ConstPrivateRef. Every overload taking a Ref reaches the entry without a lookup. Passing a Ref from another store is
// a precondition failure.
template<class SharedData>
trait DataStore [Types]
{
    requires requires { typename SharedData::Id; }

    type PrivateData // the calling listener's PrivateData, or void when the caller is not a DataListener
    type Handle
    type Ref
    // {ref, privateData}: privateData is the caller's own PrivateData, null if the ref is empty, if the caller was not
    // subscribed at the lookup, or if the caller is not a DataListener
    type PrivateRef
    type ConstPrivateRef
    type Added // {ref, privateData, inserted}

    requires std::same_as<typename Types::Handle, arc::DataHandle<SharedData>>
    requires arc::IsDataRef<typename Types::Ref, SharedData, typename Types::Handle>
    requires arc::IsDataPrivateRef<typename Types::PrivateRef, SharedData, typename Types::Ref, typename Types::PrivateData>
    requires arc::IsDataPrivateRef<typename Types::ConstPrivateRef, SharedData, typename Types::Ref, typename Types::PrivateData const>
    requires arc::IsDataAdded<typename Types::Added, SharedData, typename Types::Ref, typename Types::PrivateData>

    // Constructs the SharedData from the args, unless the id already exists, and offers the entry to the init of each
    // listener not yet subscribed to it. The entry is kept only if a listener is subscribed afterwards; otherwise the
    // returned ref is empty. inserted is whether any listener subscribed during this call.
    // When called from an init for the id being offered, add does nothing and returns the entry with
    // inserted == false; the outer add goes on to offer the entry to the remaining listeners.
    add(SharedData::Id const& id, auto&&... sharedDataArgs) -> Types::Added

    // The entry for the id, or an empty ref if the id is absent
    get(SharedData::Id const& id) -> Types::PrivateRef
    get(SharedData::Id const& id) const -> Types::ConstPrivateRef

    // The entry the handle was taken from. The ref is empty once that entry has been released, even if the id was
    // added again: a stale handle is an expected outcome, not an error.
    resolve(Types::Handle const& handle) -> Types::PrivateRef
    resolve(Types::Handle const& handle) const -> Types::ConstPrivateRef
    // The entry the Ref reads, with the caller's PrivateData. The result is empty only if the Ref is empty: an entry
    // released since the Ref was taken is still read, with a null privateData.
    resolve(Types::Ref const& ref) -> Types::PrivateRef
    resolve(Types::Ref const& ref) const -> Types::ConstPrivateRef

    // Calls useData(SharedData*, PrivateData*) and notifies the event it returns to every subscribed listener except
    // the caller. Nothing is notified if useData returns void or arc::noEvent, in which case it receives a
    // SharedData const*, nor if it returns an empty std::optional.
    // The id overload returns the entry after the listeners have handled the event: the ref is empty if the id is
    // absent or the entry was released in the meantime.
    template<std::invocable<SharedData*, typename Types::PrivateData*> UseData>
    modify(SharedData::Id const& id, UseData&& useData) -> Types::PrivateRef
    template<arc::IsDataRefFor<Types> Ref, std::invocable<SharedData*, typename Types::PrivateData*> UseData>
    modify(Ref const& ref, UseData&& useData) -> void

    // Calls useData(Id const&, SharedData*, PrivateData*) for each entry, notifying as modify does. The SharedData is
    // mutable only when useData returns an event.
    // Only the entries present when the loop began are visited: an entry added during the loop is skipped, and an
    // entry released during the loop is skipped unless it was already visited.
    template<std::invocable<typename SharedData::Id const&, SharedData*, typename Types::PrivateData*> UseData>
    forEach(UseData&& useData) -> void
    template<std::invocable<typename SharedData::Id const&, SharedData const*, typename Types::PrivateData const*> UseData>
    forEach(UseData&& useData) const -> void

    // Notifies every subscribed listener of the event for the entry.
    // The id overload returns the entry after the listeners have handled the event: the ref is empty if the id is
    // absent or the entry was released in the meantime.
    notify(auto const& event, SharedData::Id const& id) -> Types::PrivateRef
    template<arc::IsDataRefFor<Types> Ref>
    notify(auto const& event, Ref const& ref) -> void

    // Notifies the event for every entry, visiting the entries as forEach does
    notifyAll(auto const& event) -> void

    // Subscribes the calling listener to an existing entry, constructing its PrivateData from the args without calling
    // init. The ref is empty if the id is absent or the entry was released. If the listener is already subscribed, its
    // existing PrivateData is kept, not reconstructed.
    // Subscribing to the id that the listener's own init is currently being offered is a precondition failure.
    subscribe(SharedData::Id const& id, auto&&... privateDataArgs) -> Types::PrivateRef
    template<arc::IsDataRefFor<Types> Ref>
    subscribe(Ref const& ref, auto&&... privateDataArgs) -> Types::PrivateRef

    // Ends the calling listener's subscription and returns whether it was subscribed. Events stop reaching the listener
    // immediately. Once no listener is subscribed, the id is removed: get returns an empty ref and handles go stale.
    // The released PrivateData, and the SharedData once the id is removed, are destroyed when the last ref pinning the
    // entry goes, so their destructors must not call into the graph. Until the released PrivateData is destroyed, add
    // does not offer the entry to that listener, and subscribing the listener to it again is a precondition failure.
    // Optional, because only listeners subscribe, so not every caller's view requires it.
    ? release(SharedData::Id const& id) -> bool
    template<arc::IsDataRefFor<Types> Ref>
    release(Ref const& ref) -> bool
}

template<class SharedData>
trait DataListener [Types]
{
    requires requires { typename SharedData::Id; }

    type PrivateData // arc::NoPrivateData when the listener needs no per-entry state
    requires !std::is_void_v<typename Types::PrivateData>

    // Offers a newly added entry to the listener. To subscribe to every id, return the PrivateData directly.
    // To subscribe to only some ids, return void and call privateDataConstructor(args...) -> PrivateData* for the ids
    // to subscribe to: if the constructor is not called, the listener is not subscribed and no PrivateData is created.
    // The handle resolves from now on, and goes stale if no listener subscribes.
    init(SharedData::Id const& id, arc::DataHandle<SharedData> const& handle, SharedData& sharedData, auto privateDataConstructor) -> arc::detail::SameAsOrVoid<typename Types::PrivateData> auto

    // Called on each subscribed listener before its onEvent; returning true skips onEvent for this listener.
    // Without an override, no event is skipped. A useful catch-all is `return not event.isLatest();`
    template<class EventType>
    skipEvent(Event<EventType> event, EventItem<SharedData const, typename Types::PrivateData const> item) -> std::convertible_to<bool> auto
    {
        return std::false_type{};
    }

    // Called on each subscribed listener when modify, notify or notifyAll is called on the store. Returning false
    // stops the event from reaching any further listeners.
    // The SharedData is const because it changes only through DataStore::modify, which notifies.
    template<class EventType>
    onEvent(Event<EventType> event, EventItem<SharedData const, typename Types::PrivateData> item) -> arc::detail::SameAsOrVoid<bool> auto
}

} // namespace arc
