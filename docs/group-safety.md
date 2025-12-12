# Group-Based Access Control for Safety-Critical Systems

## Overview

ARC provides a compile-time group classification system that enforces access control between nodes in a graph. This feature is designed for safety-critical systems where nodes must be segregated by safety integrity levels (SIL), criticality levels, or other safety standards to prevent unauthorized interactions.

## Key Benefits

- **Compile-time enforcement**: Violations are caught at build time, not runtime
- **Zero runtime overhead**: All checks are performed during compilation
- **Flexible bidirectional permissions**: Control both outgoing and incoming connections
- **Inheritance**: Child clusters can inherit group classifications from parent contexts
- **Standards compliance**: Enforce IEC 61508, ISO 26262, DO-178C, or custom safety standards

## Basic Concepts

### Groups and Classifications

A **group** defines a set of **classifications** and the permitted connections between them. Classifications represent safety levels, trust boundaries, or any other segregation requirement.

```cpp
groups SafetyClass
{
    ASIL_D    // Highest automotive safety level
    ASIL_B    // Medium automotive safety level
    QM        // Quality managed (non-safety)

    QM -> ASIL_B    // QM nodes can connect to ASIL_B
    ASIL_B -> ASIL_D // ASIL_B nodes can connect to ASIL_D
}
```

### Connection Rules

- A node with an unspecified classification **is permitted to connect to anything**
- Explicit arrows define **permitted connections** from one classification to another
- Connections between nodes of the same classification are always permitted
- **Connections are NOT transitive**: `A -> B` and `B -> C` does not imply `A -> C`

### Permission Semantics

```cpp
Low -> High  // Low classification can connect TO High classification
```

This means:
- A Low node can call methods on a High node
- A Low node can depend on traits provided by a High node
- Information can flow FROM Low TO High

## Usage

### Defining a Group

Define groups in your ARC DSL file:

```cpp
groups SecurityLevel
{
    Untrusted
    Trusted
    Privileged

    Untrusted -> Trusted      // Untrusted can escalate to Trusted
    Trusted -> Privileged     // Trusted can escalate to Privileged
    // Note: No path from Untrusted directly to Privileged
}
```

### Applying Classifications to Nodes

Use the `WithGroup<NodeType, Classification>` wrapper to assign a classification:

```cpp
cluster SafeSystem [Root]
{
    // High integrity sensor processing
    sensor = Root::Sensor : WithGroup<SecurityLevel::Privileged>

    // Medium integrity control logic
    controller = Root::Controller : WithGroup<SecurityLevel::Trusted>

    // Low integrity user interface
    ui = Root::UserInterface : WithGroup<SecurityLevel::Untrusted>

    [trait::SensorData]
    sensor --> controller  // OK: Trusted can receive from Privileged

    [trait::ControlCommand]
    controller --> sensor  // OK: Privileged can receive from Trusted

    [trait::DisplayData]
    controller --> ui      // OK: Untrusted can receive from Trusted

    // ui --> sensor would be COMPILE ERROR: No permission path
}
```

### Applying Classifications to Clusters

Entire clusters can be assigned a classification, which all contained nodes inherit:

```cpp
cluster CriticalSubsystem
{
    node1 = Node
    node2 = Node
    // Both nodes inherit the cluster's group
}

cluster MainSystem
{
    critical = CriticalSubsystem : WithGroup<SafetyClass::ASIL_D>
    normal = Node : WithGroup<SafetyClass::QM>

    // normal cannot connect to critical.node1 (QM -> ASIL_D not permitted)
}
```

## Safety-Critical Patterns

### Automotive (ISO 26262)

```cpp
groups ASIL
{
    ASIL_D  // Highest
    ASIL_C
    ASIL_B
    ASIL_A
    QM      // Quality Managed (non-safety)

    QM -> ASIL_A -> ASIL_B -> ASIL_C -> ASIL_D
    // Lower integrity can provide data to higher integrity
    // (with appropriate validation at boundaries)
}
```

### Avionics (DO-178C)

```cpp
groups DAL  // Design Assurance Level
{
    DAL_A  // Catastrophic failure conditions
    DAL_B  // Hazardous failure conditions
    DAL_C  // Major failure conditions
    DAL_D  // Minor failure conditions
    DAL_E  // No safety effect

    DAL_E -> DAL_D -> DAL_C -> DAL_B -> DAL_A
}
```

### Industrial (IEC 61508)

```cpp
groups SIL  // Safety Integrity Level
{
    SIL_4  // Highest
    SIL_3
    SIL_2
    SIL_1
    NonSafety

    NonSafety -> SIL_1 -> SIL_2 -> SIL_3 -> SIL_4
}
```

### Multi-Level Security (MLS)

```cpp
groups Clearance
{
    TopSecret
    Secret
    Confidential
    Unclassified

    Unclassified -> Confidential -> Secret -> TopSecret
    // Information flows UP (read down, write up)
}
```

### Defense in Depth

```cpp
groups SecurityZone
{
    DMZ
    Internal
    Secured
    Critical

    DMZ -> Internal -> Secured -> Critical
    // No path from DMZ directly to Critical
}
```

## Advanced Usage

### Multiple Independent Groups

Different parts of your system can use different group classifications:

```cpp
groups SecurityLevel { /* ... */ }
groups SafetyLevel { /* ... */ }

cluster MyCluster
{
    // Node can be in both a safety AND security classification
    node = Node : WithGroup<SafetyLevel::ASIL_D, SecurityLevel::Privileged>
}
```

## Error Messages

When a violation occurs, you'll get a clear compile-time error:

```
error: static assertion failed: Cannot finalise connection: source group does not permit connections to target group
    static_assert(PermissibleNode<Source, Target>,
                  ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
```

## Limitations

- Groups are purely structural; they don't provide cryptographic guarantees
- Runtime validation must be added separately if needed
- All permissions are binary (allowed/denied); no capability-based security
- Group membership is static; nodes cannot change groups at runtime

## Summary

ARC's group system provides zero-overhead, compile-time enforcement of node segregation policies. It's designed for safety-critical and security-critical systems where preventing unauthorized interactions is essential for certification and correct operation.

By defining clear classifications and connection rules, you can ensure that your system architecture respects safety integrity levels, security zones, or any other segregation requirements mandated by your applicable standards
