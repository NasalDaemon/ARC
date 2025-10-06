# Domains

### PLEASE NOTE:
Domains in ARC are special `cluster`s with extra restrictions. If you are unfamiliar with the `cluster` syntax in ARC, please first familiarise yourself with `cluster`s and their syntax [here](cluster-syntax.md).

<details>
<summary>:eyes: TL;DR:</summary>

> **Domains** in ARC are specialized clusters designed to enforce clear boundaries, reduce coupling, and encourage a scale-free topology in your dependency graph. Scale-free topologies—where a few nodes (hubs) have many connections and most have few—make large codebases more maintainable, resilient, and easier to reason about, mirroring robust real-world networks.
>
> Domains differ from normal clusters by requiring a single nexus node (the orchestrator), restricting direct sub-node connections, and enforcing naming/wiring rules. This results in flatter, shallower, and more modular graphs, ideal for complex or growing projects.
>
> **To define a domain:**
> Use `domain` instead of `cluster`, specify a single nexus, and follow naming conventions (ALL_CAPS for sub-clusters/domains, CamelCase for stateful nodes, lowercase for stateless).
>
> Use domains when you need strict boundaries, orchestration, and scalability; use clusters for simpler groupings.
</details>

## Principal motivation

As a project grows, the structure of the dependency graph increasingly impacts maintainability and simplicity. Dependency graphs often become more complex with longer chains of dependencies, cyclic dependencies, and deeper nesting of clusters. Greater complexity makes it more difficult to add new functionality, maintain existing functionality, and generally reason about the code and explain its behaviour.

A `domain` in ARC is a special kind of `cluster` that, when used throughout the entire graph, encourages a flatter and shallower graph shape overall, with looser coupling between nodes. It demands thoughtful design in terms of delegation of responsibility into logical domains with clear boundaries. This is achieved by preferring certain kinds of dependencies and by making explicit the statefulness and arity of sub-nodes (i.e., whether a sub-node is unary or a cluster/domain).

**Note:** `domain`s will not solve all architectural problems. They encourage a graph shape that generally results in a simpler composition. They do this by restricting or highlighting particular dependency anti-patterns to prevent them from pervading the graph and the wider codebase, but there may be situations when the less restrictive `cluster` is preferable.

### Scale-free networks
In graph theory, the [scale-free](https://en.wikipedia.org/wiki/Scale-free_network) topology describes graphs with a degree distribution (distribution of number of connections per node) that follows a power law. Below is a visual example representing a scale-free topology:

[<img src="https://github.com/user-attachments/assets/7ba75f9c-02da-49ce-91a3-89be1ed98651" style="background-color:lightblue;" />](https://www.researchgate.net/publication/263268796_A_complex_network_approach_to_supply_chain_network_theory)

[Scale-free clustering (wikipedia):](https://en.wikipedia.org/wiki/Scale-free_network#Clustering):
> Another important characteristic of scale-free networks is the clustering coefficient distribution, which decreases as the node degree increases. This distribution also follows a power law. This implies that the low-degree nodes belong to very dense sub-graphs and those sub-graphs are connected to each other through hubs. Consider a social network in which nodes are people and links are acquaintance relationships between people. It is easy to see that people tend to form communities, i.e., small groups in which everyone knows everyone (one can think of such community as a complete graph). In addition, the members of a community also have a few acquaintance relationships to people outside that community. Some people, however, are connected to a large number of communities (e.g., celebrities, politicians). Those people may be considered the hubs responsible for the small-world phenomenon.

Complex networks tend to evolve towards a highly resilient scale-free topology to adapt to sporadic change and growth. For example, the shape of the internet is approximately scale-free. Online services organically become significant hubs when they not only provide high value, but are also resilient enough to meet increasing demand as the number of clients inevitably grows. For the hub to remain highly available to clients, it internally delegates units of work to a small cluster of workers. Clients in turn are able to extract maximal value from the hubs without affecting the stability of the whole network. Ideally, codebases should likewise remain robust in the face of inevitable change and growth in scope, where bugs are rarely introduced as code is added or refactored.

The fact that social networks are approximately scale-free in shape also suggests that it is a suitable topology to aspire to in the dependency graph of a large project developed by multiple teams. One can conceive of the responsibility of `clusters` and `domain`s mapping fairly neatly onto the responsibility of teams, and vice versa. The dependencies between teams would then be expressed explicitly in the ARC DSL. By clarifying coupling between teams, cross-team dependencies can be optimised to improve each team's velocity in the project without compromising the overall project's stability.

## Emergence of good topology

According to various studies, iteratively growing networks tend to become scale-free when new nodes preferentially connect to existing nodes with a higher degree (i.e., hubs connected to many other nodes). ARC `domains` aim to encourage the emergence of approximately scale-free dependency graphs through the following:

* Every `domain` must have a _single_ traitful unary nexus-node which acts as the central hub or coordinator for the `domain`
* Sub-`cluster`s or sub-`domain`s must have names in ALL_CAPS
* Unary nodes (including the nexus) in the `domain` that have state must have names starting with a capital letter (e.g., CamelCase), and stateless nodes must have names starting with a lowercase letter (e.g., pascalCase)
* The nexus-node is the only node allowed any connection to or from the parent `..` of the `domain`
  * In general, the nexus-node has no restrictions on which nodes it is connected to
* Sub-`clusters` and sub-`domain`s may connect directly to each other
  * sub-`domain`-to-sub-`domain` connections are effectively indirect nexus-to-nexus connections hosted in the greater `domain`
* Unary sub-nodes may not be connected to or from non-unary nodes such as sub-`domain`s or sub-`cluster`s
  * The only nexus-node that a unary sub-node may be connected to is the nexus-node of its own `domain`
* Any unary-to-unary connection must use strong arrows (`--->>>`) which get progressively uglier (`---->>>>`) as the total number of unary-to-unary connections in the domain increases
  * This discourages unary-to-unary sub-node connections
  * It encourages tightly-coupled unary sub-nodes to be either merged into one sub-node or put into a sub-`domain`
* All sub-nodes must have their dependencies specified via a `arc::Depends` list in the node definition, even if the list is empty
  * In clusters, this is optional, but in domains it is mandatory to ensure that nodes are held to the higher standards of the domain
* Sink traits are not allowed in domains
  * All connections between nodes in the `domain` must be explicitly defined in the `domain` block

# Domain syntax

The domain syntax is identical to the cluster syntax, with a few key differences:

* The `domain` keyword is used instead of `cluster` to introduce a `domain` block
* The first node named is the nexus-node, which will be made to conform to the responsibilities of the nexus
* Node names must be spelled correctly to represent the following properties, which are enforced in the C++ build step once all the types are known:
    * Stateless unary: pascalCase
    * Stateful unary: CamelCase
    * Non-unary (sub-domain or sub-cluster): ALL_CAPS
* Unary-to-unary sub-node connections must use strong arrows `--->>>`, all of which will be made to get stronger as the total number of these connections increases
* Sink traits are not allowed in domains

## Example
```
domain ShopDomain
{
    Api = ShopRestAPI         // `Api` is stateful nexus
    CUSTOMER = CustomerDomain // sub-domain
    ORDER = OrderDomain       // sub-domain
    DELIVERY = DeliveryDomain // sub-domain

    using req = trait::ShopRequest,         res = trait::ShopResponse
    using custReq = trait::CustomerRequest, custRes = trait::CustomerResponse
    using delReq = trait::DeliveryRequest,  delRes = trait::DeliveryResponse
    using orderReq = trait::OrderRequest,   orderRes = trait::OrderResponse

    [res      <-> req]      .. <-> Api
    [custRes  <-> custReq]         Api <-> CUSTOMER
    [delRes   <-> delReq]          Api <-> DELIVERY
    [orderRes <-> orderReq]        Api <-> ORDER

    // Order may be cancelled due to customer request, compliance or delivery problems
    [trait::OrderCancel]
    Api, CUSTOMER, DELIVERY --> ORDER
}

domain CustomerDomain
{
    gw = CustomerGateway        // `gw` is stateless nexus
    Accounts = CustomerAccounts // stateful unary sub-node
    Regs = CustomerRegulations  // stateful unary sub-node

    using res = trait::CustomerResponse, req = trait::CustomerRequest
    [res <-> req]
    .. <-> gw

    [trait::OrderCancel]
    .. <-- gw

    [trait::CustomerRegulations]
    gw --> Regs
    [trait::CustomerAccounts]
    gw --> Accounts
}
```
> [![ShopDomain](domain-syntax.svg)](domain-syntax.dot)
$\textcolor[rgb]{0,0,1}{\textsf{trait::CustomerRequest}}$
$\textcolor[rgb]{1,0,0}{\textsf{trait::CustomerResponse}}$
$\textcolor[rgb]{0,0.5,0}{\textsf{trait::DeliveryRequest}}$
$\textcolor[rgb]{1,0.647,0}{\textsf{trait::DeliveryResponse}}$
$\textcolor[rgb]{0.5,0,0.5}{\textsf{trait::OrderRequest}}$
$\textcolor[rgb]{0.647,0.165,0.165}{\textsf{trait::OrderResponse}}$
$\textcolor[rgb]{0.5,0.5,0.5}{\textsf{trait::OrderCancel}}$
$\textcolor[rgb]{0.722,0.525,0.043}{\textsf{trait::CustomerAccounts}}$
$\textcolor[rgb]{0,0.392,0}{\textsf{trait::CustomerRegulations}}$

Below is an example of the nexus-node for `ShopDomain`. In handwritten C++ code, it explicitly orchestrates multiple sub-`domain`s in order to complete an `OrderRequest`. This is much more desirable compared to the alternative pipeline approach, i.e., using a chain of nodes, each of which would take responsibility for completing a part of the request. By spreading out the responsibility across multiple nodes, a chain of nodes makes it fundamentally unclear which node has ownership of: (1) completing the request and (2) producing the response. Pipelines tend to be difficult to test for correctness, as one often needs to construct large sections of the pipeline, if not the whole thing, to get meaningful behaviour worth testing. This tight coupling necessitates integration-style testing of the pipeline and prohibits granular unit testing.

Instead, `ShopRestAPI` takes ownership of the entire flow of the order, delegating the responsibility for the smaller self-contained units of work to sub-nodes which take no responsibility for completing the overall `OrderRequest`. This looser form of coupling also allows for finer-grained testing, as each testable unit of behaviour is isolated within fewer sub-`domain`s and sub-nodes.

```cpp
struct ShopRestAPI
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<
              trait::ShopResponse
            , trait::CustomerRequest
            , trait::DeliveryRequest
            , trait::OrderRequest
            , trait::OrderCancel
        >;

        using Traits = arc::Traits<Node
            , trait::ShopRequest
            , trait::CustomerResponse
            , trait::DeliveryResponse
        >;

        using OrderRequest = Context::Root::OrderRequestType;
        using RequestId = std::size_t;

        void impl(trait::ShopRequest::order, OrderRequest req)
        {
            // Gateway coordinates the order flow by delegating to the other nodes
            newOrderFlows[req.id].req = std::move(req);
            scheduleTimeout(req.id);
            getNode(trait::customerRequest).canAfford(req.id, req.customerId, req.productId, req.quantity);
            getNode(trait::deliveryRequest).canDeliver(req.id, req.productId, req.quantity);
        }

        void scheduleTimeout(RequestId reqId)
        {
            // ... logic to respond with timeout if not all subdomains respond in time
        }

        void impl(trait::CustomerResponse::canAfford, RequestId reqId, bool canAfford)
        {
            updateFlow(reqId, &NewOrderFlow::canAfford, canAfford);
        }

        void impl(trait::DeliveryResponse::canDeliver, RequestId reqId, bool canDeliver)
        {
            updateFlow(reqId, &NewOrderFlow::canDeliver, canDeliver);
        }

        template<class T>
        void updateFlow(RequestId reqId, T NewOrderFlow::* memPtr, auto const& value)
        {
            if (auto flowIt = newOrderFlows.find(reqId); it != newOrderFlows.end())
            {
                auto& flow = flowIt->second;
                flow.*memPtr = value;
                // Respond to client if we have received all responses or a failure condition
                if (flow.readyToRespond())
                {
                    cancelTimeout(reqId);
                    onResponseReady(flow);
                    newOrderFlows.erase(flowIt);
                }
            }
        }

        void onResponseReady(NewOrderFlow const& flow)
        {
            ShopRespose res;
            if (not flow.canAfford.value_or(true))
            {
                res.success = false;
                res.failureReason = "Not enough funds";
            }
            else if (not flow.canDeliver.value_or(true))
            {
                res.success = false;
                res.failureReason = "Not enough stock";
            }
            else
            {
                // If we are responding with success, start the order in the backend
                res.success = true;
                res.orderId = getNode(trait::orderRequest).startOrder(
                    flow.req.id,
                    flow.req.customerId,
                    flow.req.productId,
                    flow.req.quantity);
            }

            getNode(trait::shopResponse).order(reqId, res);
        }

        // ... etc

        struct NewOrderFlow
        {
            OrderRequest req;
            std::optional<bool> canAfford;
            std::optional<bool> canDeliver;

            bool readyToRespond() const
            {
                // We are ready to respond if we have received both canAfford and canDeliver responses
                // or if one of them is false, indicating a failure condition
                return canAfford.has_value() and canDeliver.has_value()
                    or (not canAfford.value_or(true) or not canDeliver.value_or(true));
            }
        };

        std::map<RequestId, NewOrderFlow> newOrderFlows;
    };
}
```

## When to use domains instead of clusters

See a chat with Copilot [here](copilot/domain-refactor.md) which helps to explain when domains should be chosen over clusters and vice versa.
