# Policy-Based Access Control

## Overview

ARC provides compile-time policy enforcement for node graph access control. Designed for safety-critical systems requiring segregation by safety integrity levels (SIL), security zones, or other standards.

**Key Benefits:**
- **Compile-time enforcement** - Violations caught at build time with zero runtime overhead
- **Flexible permissions** - Read-only or read-write access, unidirectional or bidirectional
- **Inheritance** - Clusters inherit group classifications from parent contexts
- **Standards compliance** - Enforce IEC 61508, ISO 26262, DO-178C, or custom policies

## Core Concepts

### Policies and Groups

A **policy** defines **groups** and their permitted connections:

```cpp
policy SafetyClass
{
    ASIL_D    // Highest automotive safety level
    ASIL_B    // Medium level
    QM        // Quality managed (non-safety)

    QM -> ASIL_B     // QM can read from ASIL_B
    ASIL_B => ASIL_D // ASIL_B can write to ASIL_D
}
```

### Arrow Types

| Arrow | Meaning | Equivalent |
|-------|---------|------------|
| `A -> B` | A can **read** B | - |
| `A => B` | A can **read-write** B | - |
| `A <-> B` | Bidirectional read | `A -> B` + `B -> A` |
| `A <=> B` | Bidirectional read-write | `A => B` + `B => A` |
| `A <=\|-> B` | A reads B, B writes A | `A -> B` + `B => A` |
| `A <-\|=> B` | A writes B, B reads A | `A => B` + `B -> A` |

**Note:** There is no write-only access - write (`=>`) always includes read.

### Connection Rules

- Same-group connections are always permitted (with write access)
- Different-group connections require explicit arrows
- **Connections are NOT transitive**: `A -> B` and `B -> C` does not imply `A -> C`
- Nodes without a group use the **default group** (`@nogroup` or `~`)
- Without explicit `@nogroup` rules, unclassified nodes only connect to each other

### Multiple Group Membership

Nodes can belong to multiple groups:

```cpp
node = Root::Default : InGroup<Class::High, Class::Low>
```

Permission semantics for JoinedGroups:
- **Reading**: Need permission to read from **any one** constituent group (OR)
- **Writing**: Need permission to write to **all** constituent groups (AND)

```cpp
policy Access
{
    Admin
    User
    Guest

    Guest -> User     // Guest can read User
    Admin => User     // Admin can write to User
}

// Node in both User and Admin groups
multiNode = Node : InGroup<Access::User, Access::Admin>

// Guest can READ multiNode (Guest -> User satisfies OR)
// Guest CANNOT WRITE to multiNode (no write to Admin, fails AND)
// Admin can WRITE to multiNode (can write to both groups)
```

## Usage

### Defining Policies

```cpp
policy SecurityLevel
{
    Untrusted
    Trusted
    Privileged

    Untrusted -> Trusted -> Privileged  // Read up the chain
    Untrusted <= Trusted <= Privileged  // Write down the chain
}
```

**Group aliases** allow linking policies:

```cpp
policy CombinedPolicy
{
    Public
    Confidential
    External = SecurityLevel::Untrusted  // Alias to another policy

    Public -> Confidential
    External => Public  // External can write to Public
}
```

### Applying to Nodes

```cpp
cluster SafeSystem [Root]
{
    sensor     = Root::Sensor        : InGroup<SecurityLevel::Privileged>
    controller = Root::Controller    : InGroup<SecurityLevel::Trusted>
    ui         = Root::UserInterface : InGroup<SecurityLevel::Untrusted>

    [trait::SensorData]
    sensor --> controller  // OK: Trusted reads from Privileged

    [trait::DisplayData]
    controller --> ui      // OK: Trusted writes to Untrusted
    ui --> controller      // OK: Untrusted reads from Trusted

    // ui --> sensor would be COMPILE ERROR: No permission path
}
```

### Applying to Clusters

Clusters can inherit a classification for all contained nodes:

```cpp
policy SafetyClass
{
    ASIL_D
    ASIL_C
    QM

    QM -> ASIL_C -> ASIL_D  // Read up
    ASIL_D => ASIL_C => QM  // Write down
}

cluster MainSystem
{
    critical = CriticalSubsystem : InGroup<SafetyClass::ASIL_D>
    normal = Node : InGroup<SafetyClass::QM>

    // normal can read from critical.node1 (QM -> ASIL_C -> ASIL_D)
    // critical.node1 can write to normal (ASIL_D => ASIL_C => QM)
}

cluster CriticalSubsystem
{
    node1 = Node  // Inherits ASIL_C from MainSystem.critical
    node2 = Node  // Inherits ASIL_C from MainSystem.critical
}
```

## Safety-Critical Patterns

Common patterns for safety and security standards. All follow the principle: **lower integrity reads from higher, higher integrity writes to lower**.

### Automotive (ISO 26262 - ASIL)

```cpp
policy ASIL
{
    ASIL_D  // Highest
    ASIL_C
    ASIL_B
    ASIL_A
    QM      // Quality Managed

    // Explicit form:
    QM -> ASIL_A -> ASIL_B -> ASIL_C -> ASIL_D  // Read up
    ASIL_D => ASIL_C => ASIL_B => ASIL_A => QM  // Write down

    // Or compact bidirectional form:
    // ASIL_D <-|=> ASIL_C <-|=> ASIL_B <-|=> ASIL_A <-|=> QM
}
```

### Avionics (DO-178C - DAL)

```cpp
policy DAL
{
    DAL_A  // Catastrophic
    DAL_B  // Hazardous
    DAL_C  // Major
    DAL_D  // Minor
    DAL_E  // No effect

    DAL_A <-|=> DAL_B <-|=> DAL_C <-|=> DAL_D <-|=> DAL_E
}
```

### Industrial (IEC 61508 - SIL)

```cpp
policy SIL
{
    SIL_4
    SIL_3
    SIL_2
    SIL_1
    NonSafety

    SIL_4 <-|=> SIL_3 <-|=> SIL_2 <-|=> SIL_1 <-|=> NonSafety
}
```

### Defense in Depth

```cpp
policy SecurityZone
{
    Critical
    Secured
    Internal
    DMZ

    // Inner zones read from outer, write to outer
    Critical <-|=> Secured <-|=> Internal <-|=> DMZ
    // No direct path from DMZ to Critical
}
```

### MLS Note

Classic Multi-Level Security ("no read up, no write down") cannot be directly represented because ARC has no write-only access - C++ cannot enforce write-only semantics. You can model read-only data flow:

```cpp
policy Clearance
{
    Unclassified
    Confidential
    Secret
    TopSecret

    TopSecret -> Secret -> Confidential -> Unclassified  // Read down
}
```

For true MLS, use dedicated security frameworks with runtime access controls.

## Multiple Independent Policies

Different parts of a system can use different policy classifications:

```cpp
policy SecurityLevel { /* ... */ }
policy SafetyLevel { /* ... */ }

cluster MyCluster
{
    node = Node : InGroup<SafetyLevel::ASIL_D, SecurityLevel::Privileged>
}
```

## Error Messages

Violations produce clear compile-time errors:

```
error: static assertion failed: Cannot finalise connection: source group does not permit connections to target group
```

## Limitations

- Policies are structural only (no cryptographic guarantees)
- Runtime validation must be added separately if needed
- Permissions are binary (allowed/denied); no capability-based security
- Group membership is static (nodes cannot change groups at runtime)

## Summary

ARC's policy system provides zero-overhead, compile-time enforcement of node segregation. By defining policies with group classifications and connection rules, you ensure your architecture respects safety integrity levels, security zones, or other segregation requirements mandated by applicable standards.
