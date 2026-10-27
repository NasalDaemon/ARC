namespace arc {

trait CollectionControl [Types]
{
    type ID
    type WeakHandle
    type StrongHandle

    contains(Types::ID const& id) const -> bool
    contains(Types::StrongHandle const& handle) const -> bool
        pre (handle)

    getWeakHandle(Types::ID const& id) const -> Types::WeakHandle
    getStrongHandle(Types::ID const& id) const -> Types::StrongHandle
    getId(Types::StrongHandle const& handle) const -> Types::ID
        pre (handle)

    // Get TraitView, const through a const view
    fromId(Types::ID const& id)
    fromId(Types::ID const& id) const
    fromHandle(Types::StrongHandle const& handle)
        pre (handle)
    fromHandle(Types::StrongHandle const& handle) const
        pre (handle)

    // f: (ID, StrongHandle, TraitView) -> void
    forEach(auto&& f) -> void
    forEach(auto&& f) const -> void
    // f: (ID, StrongHandle, TraitView) -> bool
    findIf(auto&& f) -> Types::StrongHandle
    findIf(auto&& f) const -> Types::StrongHandle
}

trait DynamicCollectionControlExt [Types]
{
    type ID
    type InsertId
    type WeakHandle
    type StrongHandle

    // Index collections take arc::autoId here (InsertId), map collections take an ID.
    tryInsert(Types::InsertId const& id, auto&&... args) -> std::pair<typename Types::StrongHandle, bool>

    remove(Types::ID const& id) -> bool
    remove(Types::StrongHandle const& handle) -> bool

    // pred: (ID, StrongHandle, TraitView) -> bool
    removeIf(auto&& pred) -> void
}

trait DynamicCollectionControl = CollectionControl + DynamicCollectionControlExt

} // namespace arc
