# Resolving into a cluster that is being instantiated

A node inside a subcluster cannot name, at class scope, the types of a node whose resolution goes back into that
subcluster. A DataStore is the case that meets it:

```arc
cluster Sub [R = Root]
{
    inner = R::Probe
    side = R::Side

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    .. <-> inner

    [arc::DataStore<typename R::Data>]
    side --> ..
}
```

With `store <-> sub` in the parent, `side` can name `arc::ResolveTypes<Node, DataStore>::Ref` inside a member
function, but not in its class body.

## Cause

`side`'s class body is instantiated with `Sub::Node`. Naming the store's types resolves its `DataListener` link into
`Sub::Node`, whose `resolveLink` is not yet visible as the class is incomplete, so resolution stops at `Sub::Node`.

## Fix

Declare a cluster's member contexts and parent links on a `Sub::Links<C>` complete before `Sub::Node<C>`, and resolve
links through it in `generator_lib/template.jinja`, `detail::LocallyResolvable` and `detail::ResolveLink`.
