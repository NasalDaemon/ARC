namespace arc::trait {

template<class SharedData>
trait DataStore [Types]
{
    requires typename SharedData::Id

    add(SharedData::Id const& id, auto&&... sharedDataArgs) -> bool
    get(SharedData::Id const& id) // -> std::pair<SharedData*, PrivateData*>

    notify(auto const& event, SharedData::Id const& id) -> SharedData const*
    notifyAll(auto const& event) -> void
}

template<class SharedData>
trait DataListener [Types]
{
    requires typename SharedData::Id
    type PrivateData

    init(SharedData::Id const& id, SharedData& sharedData, auto privateDataConstructor) -> void

    onEvent(auto const& event, SharedData::Id const& id, SharedData& sharedData, Types::PrivateData& privateData) -> void
}

} // namespace arc::trait
