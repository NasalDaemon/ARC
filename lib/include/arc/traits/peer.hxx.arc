trait arc::trait::Peer [Info]
{
    info ElementContext::IdType

    isPeerId(Info::ElementContext::IdType const& id) const -> std::convertible_to<bool> auto
    isPeerInstance(auto const& peerState) const -> std::convertible_to<bool> auto
}
