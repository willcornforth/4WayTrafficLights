# Formal Methods Specification: 4-Way Traffic Light Control System

## Document Information

| Field | Value |
|---|---|
| System | 4-Way Junction Traffic Light Controller |
| Safety Integrity Level | High (Safety-Critical) |
| Implementation Language | C (High-Integrity subset) |
| Document Version | 1.0 |
| Date | 2026-03-23 |

---

## Table of Contents

1. [System Description](#1-system-description)
2. [Definitions and Notation](#2-definitions-and-notation)
3. [Formal State Model](#3-formal-state-model)
4. [State Transition Specification](#4-state-transition-specification)
5. [Safety Invariants](#5-safety-invariants)
6. [Temporal Properties](#6-temporal-properties)
7. [Fault Detection and Handling](#7-fault-detection-and-handling)
8. [Pedestrian Subsystem Specification](#8-pedestrian-subsystem-specification)
9. [High-Integrity C Implementation Constraints](#9-high-integrity-c-implementation-constraints)
10. [Traceability Matrix](#10-traceability-matrix)

---

## 1. System Description

### 1.1 Overview

The system controls traffic lights at a standard UK 4-way junction formed by the intersection of two perpendicular roads. Each road has two lanes (one per direction of travel), yielding four traffic light installations in total. These installations are grouped into two **paired sets** that operate synchronously:

- **Pair A (North–South):** Installations controlling the two opposing lanes of one road.
- **Pair B (East–West):** Installations controlling the two opposing lanes of the perpendicular road.

### 1.2 Physical Components Per Installation

Each of the four installations comprises:

| Component | Description |
|---|---|
| Red light | Stop signal |
| Amber light | Caution / transition signal |
| Green light | Proceed signal |
| Traffic sensor | Detects vehicles waiting at the stop line |
| Pedestrian crossing subsystem | Button, crossing light, audible alert |

### 1.3 Operational Summary

The system cycles traffic between pairs. At any given time at most one pair may show a permissive signal (Green or transitional Amber). A pedestrian phase may intervene between cycles, during which all traffic is held at Red.

---

## 2. Definitions and Notation

### 2.1 Conventions

Formal specifications in this document use a combination of:

- **Z-notation style** set/type definitions for the static data model.
- **Linear Temporal Logic (LTL)** for temporal safety and liveness properties.
- **Pre/post-condition contracts** for state transition functions.

Standard logical symbols:

| Symbol | Meaning |
|---|---|
| $\land$ | Logical AND |
| $\lor$ | Logical OR |
| $\lnot$ | Logical NOT |
| $\Rightarrow$ | Implies |
| $\Leftrightarrow$ | If and only if |
| $\forall$ | For all |
| $\exists$ | There exists |
| $\square$ | Always (LTL: globally) |
| $\lozenge$ | Eventually (LTL: future) |
| $\bigcirc$ | Next state (LTL) |
| $\mathcal{U}$ | Until (LTL) |

### 2.2 Type Definitions

$$
\text{SignalColour} ::= \text{RED} \mid \text{RED\_AMBER} \mid \text{GREEN} \mid \text{AMBER} \mid \text{OFF}
$$

$$
\text{PairID} ::= \text{PAIR\_A} \mid \text{PAIR\_B}
$$

$$
\text{SystemMode} ::= \text{NORMAL} \mid \text{PEDESTRIAN\_PHASE} \mid \text{ALL\_RED\_HOLD} \mid \text{FAULT}
$$

$$
\text{SensorStatus} ::= \text{OPERATIONAL} \mid \text{FAILED}
$$

$$
\text{PedestrianRequest} ::= \text{NONE} \mid \text{PENDING} \mid \text{ACTIVE}
$$

### 2.3 System State Schema

The complete system state $S$ is defined as:

$$
S = \langle \sigma_A, \sigma_B, m, sens_A, sens_B, ped, t_{phase}, t_{ped} \rangle
$$

Where:

| Variable | Type | Description |
|---|---|---|
| $\sigma_A$ | $\text{SignalColour}$ | Current signal displayed by Pair A |
| $\sigma_B$ | $\text{SignalColour}$ | Current signal displayed by Pair B |
| $m$ | $\text{SystemMode}$ | Current operating mode |
| $sens_A$ | $\text{SensorStatus}$ | Status of Pair A's traffic sensors |
| $sens_B$ | $\text{SensorStatus}$ | Status of Pair B's traffic sensors |
| $ped$ | $\text{PedestrianRequest}$ | Pedestrian crossing state |
| $t_{phase}$ | $\mathbb{N}$ | Time elapsed in current signal phase (ms) |
| $t_{ped}$ | $\mathbb{N}$ | Time elapsed in pedestrian phase (ms) |

---

## 3. Formal State Model

### 3.1 Signal State Machine

Each paired installation follows a strict cyclic state machine. The valid signal progression for a single pair is:

$$
\text{RED} \xrightarrow{} \text{RED\_AMBER} \xrightarrow{} \text{GREEN} \xrightarrow{} \text{AMBER} \xrightarrow{} \text{RED}
$$

Formally, the **next-signal function** $\text{next} : \text{SignalColour} \rightarrow \text{SignalColour}$ is defined as:

$$
\text{next}(s) = \begin{cases}
\text{RED\_AMBER} & \text{if } s = \text{RED} \\
\text{GREEN} & \text{if } s = \text{RED\_AMBER} \\
\text{AMBER} & \text{if } s = \text{GREEN} \\
\text{RED} & \text{if } s = \text{AMBER} \\
\text{undefined} & \text{if } s = \text{OFF}
\end{cases}
$$

**Property:** The signal function is total over the operational domain $\{\text{RED}, \text{RED\_AMBER}, \text{GREEN}, \text{AMBER}\}$ and cyclic with period 4.

### 3.2 Valid Signal Combinations

At the system level, only specific combinations of $(\sigma_A, \sigma_B)$ are permitted. The **valid state set** $V$ is:

$$
V = \{(\sigma_A, \sigma_B) \mid \lnot(\text{permissive}(\sigma_A) \land \text{permissive}(\sigma_B))\}
$$

Where the **permissive predicate** is:

$$
\text{permissive}(s) \Leftrightarrow s \in \{\text{GREEN}, \text{AMBER}, \text{RED\_AMBER}\}
$$

Enumerating, the complete set of valid combinations is:

| $\sigma_A$ | $\sigma_B$ | Valid | Rationale |
|---|---|---|---|
| RED | RED | Yes | All-red hold / transition |
| RED | RED_AMBER | Yes | Pair B preparing |
| RED | GREEN | Yes | Pair B has right of way |
| RED | AMBER | Yes | Pair B clearing |
| RED_AMBER | RED | Yes | Pair A preparing |
| GREEN | RED | Yes | Pair A has right of way |
| AMBER | RED | Yes | Pair A clearing |
| GREEN | GREEN | **No** | Conflicting movements |
| GREEN | AMBER | **No** | Conflicting movements |
| AMBER | GREEN | **No** | Conflicting movements |
| AMBER | AMBER | **No** | Conflicting movements |
| RED_AMBER | RED_AMBER | **No** | Conflicting movements |
| RED_AMBER | GREEN | **No** | Conflicting movements |
| GREEN | RED_AMBER | **No** | Conflicting movements |
| RED_AMBER | AMBER | **No** | Conflicting movements |
| AMBER | RED_AMBER | **No** | Conflicting movements |
| OFF | OFF | Yes | Fault shutdown state |

**Constraint:** Any state containing OFF must have both pairs OFF:

$$
(\sigma_A = \text{OFF} \lor \sigma_B = \text{OFF}) \Rightarrow (\sigma_A = \text{OFF} \land \sigma_B = \text{OFF})
$$

### 3.3 Phase Timing Constants

| Constant | Symbol | Value | Unit |
|---|---|---|---|
| Intermediate signal duration | $T_{inter}$ | 1500 | ms |
| Green with waiting traffic | $T_{green}$ | 30000 | ms |
| Green without waiting traffic | $T_{green\infty}$ | $\infty$ (unbounded) | ms |
| Pedestrian crossing hold | $T_{ped}$ | 15000 | ms |
| Sensor poll interval | $T_{poll}$ | 100 | ms |
| Fault detection timeout | $T_{fault}$ | 500 | ms |

### 3.4 Initial State

$$
S_0 = \langle \text{RED}, \text{RED}, \text{NORMAL}, \text{OPERATIONAL}, \text{OPERATIONAL}, \text{NONE}, 0, 0 \rangle
$$

The system always starts in an all-red state, ensuring no traffic is permitted until the controller has completed self-checks and explicitly transitions to an operational cycle.

---

## 4. State Transition Specification

### 4.1 Master Cycle

The system operates a **master cycle** that alternates green phases between pairs. The cycle, starting from an all-red state with Pair A next to receive green, proceeds:

$$
\underbrace{\text{A:RED, B:RED}}_{\text{check pedestrian}} \rightarrow
\underbrace{\text{A:RED\_AMBER, B:RED}}_{T_{inter}} \rightarrow
\underbrace{\text{A:GREEN, B:RED}}_{T_{green} \text{ or } T_{green\infty}} \rightarrow
\underbrace{\text{A:AMBER, B:RED}}_{T_{inter}} \rightarrow
\underbrace{\text{A:RED, B:RED}}_{\text{check pedestrian}} \rightarrow
\underbrace{\text{A:RED, B:RED\_AMBER}}_{T_{inter}} \rightarrow
\underbrace{\text{A:RED, B:GREEN}}_{T_{green} \text{ or } T_{green\infty}} \rightarrow
\underbrace{\text{A:RED, B:AMBER}}_{T_{inter}} \rightarrow
\text{repeat}
$$

### 4.2 Transition Pre/Post Conditions

Each transition function is specified with explicit pre-conditions ($\text{pre}$), post-conditions ($\text{post}$), and the safety invariant that must hold ($\text{inv}$).

#### 4.2.1 Transition: Begin Green Phase for Pair $P$

Let $Q$ denote the opposing pair.

$$
\textbf{pre:} \quad \sigma_P = \text{RED} \land \sigma_Q = \text{RED} \land m = \text{NORMAL} \land ped \neq \text{ACTIVE}
$$

$$
\textbf{post:} \quad \sigma_P = \text{RED\_AMBER} \land \sigma_Q = \text{RED} \land t_{phase} = 0
$$

$$
\textbf{inv:} \quad \lnot(\text{permissive}(\sigma_P) \land \text{permissive}(\sigma_Q))
$$

#### 4.2.2 Transition: RED_AMBER → GREEN

$$
\textbf{pre:} \quad \sigma_P = \text{RED\_AMBER} \land t_{phase} \geq T_{inter}
$$

$$
\textbf{post:} \quad \sigma_P = \text{GREEN} \land t_{phase} = 0
$$

#### 4.2.3 Transition: GREEN → AMBER

Two triggering conditions exist:

$$
\textbf{pre:} \quad \sigma_P = \text{GREEN} \;\land\; \Big(
(t_{phase} \geq T_{green} \land \text{waiting}(Q)) \;\lor\; 
(t_{phase} \geq T_{green} \land sens_Q = \text{FAILED}) \;\lor\;
(ped = \text{PENDING})
\Big)
$$

$$
\textbf{post:} \quad \sigma_P = \text{AMBER} \land t_{phase} = 0
$$

Where:

$$
\text{waiting}(P) \Leftrightarrow \text{traffic sensor for pair } P \text{ reports vehicle present}
$$

#### 4.2.4 Transition: AMBER → RED

$$
\textbf{pre:} \quad \sigma_P = \text{AMBER} \land t_{phase} \geq T_{inter}
$$

$$
\textbf{post:} \quad \sigma_P = \text{RED} \land t_{phase} = 0
$$

#### 4.2.5 Green Hold (No Waiting Traffic)

If the intersecting road sensor reports no vehicles and the sensor is operational, the green phase extends indefinitely:

$$
\sigma_P = \text{GREEN} \land \lnot\text{waiting}(Q) \land sens_Q = \text{OPERATIONAL} \land ped = \text{NONE} \Rightarrow \sigma_P' = \text{GREEN}
$$

This is not a transition but a **hold condition** — the state does not change.

#### 4.2.6 Sensor Failure Override

$$
sens_Q = \text{FAILED} \Rightarrow \text{the green duration is bounded by } T_{green}
$$

In plain terms: when a sensor has failed, the system cannot determine whether the opposing road has waiting traffic, so it conservatively enforces the 30-second limit.

### 4.3 Transition Validation Function

Before any transition is committed, a **validation function** checks the proposed next state:

$$
\text{validate}(\sigma_A', \sigma_B', m') = \begin{cases}
\text{true} & \text{if } (\sigma_A', \sigma_B') \in V \land \text{mode\_consistent}(m') \\
\text{false} & \text{otherwise}
\end{cases}
$$

If $\text{validate}$ returns false, the transition is **rejected** and the system enters $\text{FAULT}$ mode. This provides a runtime safety net against logic errors.

---

## 5. Safety Invariants

These are properties that must hold in **every** reachable state of the system. They are expressed as universally quantified predicates over states and as LTL globally ($\square$) properties.

### INV-1: No Conflicting Permissive Signals

$$
\square\;\lnot\big(\text{permissive}(\sigma_A) \land \text{permissive}(\sigma_B)\big)
$$

*No two intersecting pairs shall simultaneously display a signal that permits or implies permission for traffic to proceed.*

**Verification:** Checked at every state transition by the validation function. Also verified at every sensor poll cycle by independently reading hardware state and comparing against the software model.

### INV-2: No Traffic During Pedestrian Crossing

$$
\square\;\Big(m = \text{PEDESTRIAN\_PHASE} \Rightarrow (\sigma_A = \text{RED} \land \sigma_B = \text{RED})\Big)
$$

*While the pedestrian phase is active, both pairs must display RED.*

### INV-3: Paired Installation Synchronisation

$$
\square\;\big(\sigma_{A1} = \sigma_{A2} \land \sigma_{B1} = \sigma_{B2}\big)
$$

Where $\sigma_{A1}, \sigma_{A2}$ are the two individual installations within Pair A, etc.

*Both installations within a pair must display identical signals at all times.*

**Implementation note:** Paired installations are driven by the same output register. The software writes one command per pair, and hardware feedback from both installations is compared to confirm synchronisation.

### INV-4: Fault State Consistency

$$
\square\;\Big(m = \text{FAULT} \Rightarrow (\sigma_A = \text{OFF} \land \sigma_B = \text{OFF})\Big)
$$

*In fault mode, all lights must be extinguished.*

### INV-5: OFF State Symmetry

$$
\square\;\Big((\sigma_A = \text{OFF} \lor \sigma_B = \text{OFF}) \Rightarrow (\sigma_A = \text{OFF} \land \sigma_B = \text{OFF})\Big)
$$

*If any pair enters the OFF state, all pairs must be OFF. Partial shutdown is not permitted.*

### INV-6: Monotonic Signal Progression

$$
\square\;\Big(m = \text{NORMAL} \Rightarrow \big(\sigma_P' = \text{next}(\sigma_P) \lor \sigma_P' = \sigma_P \lor \sigma_P' = \text{OFF}\big)\Big)
$$

*In normal mode, a signal may only advance to its defined successor, hold its current value, or transition to OFF (fault). No arbitrary jumps are permitted.*

### INV-7: Alert on Sensor Failure

$$
\square\;\Big((sens_A = \text{FAILED} \lor sens_B = \text{FAILED}) \Rightarrow \text{alert\_raised}\Big)
$$

*Whenever a sensor is in a failed state, the monitoring system must have an active alert.*

### INV-8: Pedestrian Alert During Crossing

$$
\square\;\Big(m = \text{PEDESTRIAN\_PHASE} \Rightarrow (\text{ped\_light\_on} \land \text{ped\_audible\_on})\Big)
$$

*During the pedestrian phase, the crossing light and audible alert must both be active.*

---

## 6. Temporal Properties

Beyond invariants (which are point-in-time properties), the system must satisfy **temporal** properties that describe behaviour over sequences of states.

### 6.1 Liveness: Green Eventually Granted

$$
\square\;\Big(\text{waiting}(A) \Rightarrow \lozenge\;(\sigma_A = \text{GREEN})\Big)
$$

$$
\square\;\Big(\text{waiting}(B) \Rightarrow \lozenge\;(\sigma_B = \text{GREEN})\Big)
$$

*If traffic is waiting at a pair, that pair will eventually receive a green signal (no starvation).*

### 6.2 Liveness: Pedestrian Eventually Served

$$
\square\;\Big(ped = \text{PENDING} \Rightarrow \lozenge\;(m = \text{PEDESTRIAN\_PHASE})\Big)
$$

*A pending pedestrian request will eventually lead to a pedestrian crossing phase.*

### 6.3 Bounded Response: Intermediate Signal Duration

$$
\square\;\Big(\sigma_P = \text{RED\_AMBER} \Rightarrow \lozenge_{\leq T_{inter}}\;(\sigma_P = \text{GREEN} \lor \sigma_P = \text{OFF})\Big)
$$

$$
\square\;\Big(\sigma_P = \text{AMBER} \Rightarrow \lozenge_{\leq T_{inter}}\;(\sigma_P = \text{RED} \lor \sigma_P = \text{OFF})\Big)
$$

*Intermediate signals are held for exactly $T_{inter}$ and then advance (or the system faults).*

### 6.4 Bounded Response: Green Signal Duration Under Demand

$$
\square\;\Big((\sigma_P = \text{GREEN} \land \text{waiting}(Q)) \Rightarrow \lozenge_{\leq T_{green}}\;(\sigma_P = \text{AMBER} \lor \sigma_P = \text{OFF})\Big)
$$

*When opposing traffic is waiting, green will not exceed $T_{green}$.*

### 6.5 Bounded Response: Pedestrian Phase Duration

$$
\square\;\Big(m = \text{PEDESTRIAN\_PHASE} \Rightarrow \lozenge_{\leq T_{ped}}\;(m = \text{NORMAL} \lor m = \text{FAULT})\Big)
$$

*The pedestrian phase lasts at most $T_{ped}$ before returning to normal operation.*

### 6.6 Safety: All-Red Before Any Green

$$
\square\;\Big(\sigma_P = \text{AMBER} \Rightarrow \bigcirc\;(\sigma_P = \text{RED})\;\mathcal{U}\;(\sigma_P = \text{RED\_AMBER})\Big)
$$

*After amber, the system must pass through a red state before any red-amber/green transition.*

### 6.7 Fairness: Alternation Under Balanced Demand

$$
\square\;\Big((\text{waiting}(A) \land \text{waiting}(B)) \Rightarrow \lnot\;\square\;(\text{active\_pair} = A)\Big)
$$

*When both roads have waiting traffic, neither pair is permanently favoured.*

---

## 7. Fault Detection and Handling

### 7.1 Fault Sources

The specification identifies five categories of detectable fault:

| ID | Fault | Detection Method | Response |
|----|-------|-----------------|----------|
| F1 | Traffic sensor failure | Sensor heartbeat timeout or out-of-range reading | Revert to $T_{green}$ timed mode; raise alert |
| F2 | Signal fails to advance | Hardware feedback differs from commanded state after $T_{fault}$ | All OFF; raise alert |
| F3 | Light fails to illuminate | Read-back of hardware output shows light not on | All OFF; raise alert |
| F4 | Light fails to de-illuminate | Read-back of hardware output shows light still on after command to extinguish | All OFF; raise alert |
| F5 | Paired installations desynchronised | Cross-check hardware feedback of both installations in a pair | All OFF; raise alert |

### 7.2 Formal Fault Response

For critical faults (F2–F5), the response is immediate entry to fault mode:

$$
\text{fault\_detected}(F) \land F \in \{F2, F3, F4, F5\} \Rightarrow \bigcirc\;(m = \text{FAULT} \land \sigma_A = \text{OFF} \land \sigma_B = \text{OFF} \land \text{alert\_raised})
$$

For degraded-mode faults (F1):

$$
\text{fault\_detected}(F1) \Rightarrow \bigcirc\;\Big(\text{green\_duration\_limit} = T_{green} \land \text{alert\_raised} \land m = \text{NORMAL}\Big)
$$

### 7.3 Hardware Feedback Verification

After every command to change a signal, the system reads back the actual hardware state and verifies it matches the commanded state within $T_{fault}$:

$$
\text{command}(\sigma_P, s) \Rightarrow \lozenge_{\leq T_{fault}}\;\Big(\text{hw\_readback}(\sigma_P) = s \lor m = \text{FAULT}\Big)
$$

This implements a **command–confirm** pattern: every write is followed by a read-back check.

### 7.4 Sensor Health Monitoring

Each traffic sensor must produce a heartbeat (regular status message) within a defined period:

$$
\square\;\Big(\text{last\_heartbeat}(sens_P) > T_{sensor\_timeout} \Rightarrow sens_P = \text{FAILED}\Big)
$$

A sensor returning to operational status requires multiple consecutive valid readings to avoid spurious recovery:

$$
sens_P = \text{FAILED} \land \text{consecutive\_valid\_readings}(sens_P) \geq N_{recovery} \Rightarrow sens_P' = \text{OPERATIONAL}
$$

Where $N_{recovery}$ is a design constant (recommended: 10 consecutive readings at $T_{poll}$ interval = 1 second of stable readings).

---

## 8. Pedestrian Subsystem Specification

### 8.1 Pedestrian Request State Machine

$$
\text{NONE} \xrightarrow{\text{button press}} \text{PENDING} \xrightarrow{\text{all red reached}} \text{ACTIVE} \xrightarrow{T_{ped} \text{ elapsed}} \text{NONE}
$$

### 8.2 Pedestrian Phase Entry Condition

A pedestrian phase may only begin when all traffic is held:

$$
\textbf{pre:} \quad ped = \text{PENDING} \land \sigma_A = \text{RED} \land \sigma_B = \text{RED}
$$

$$
\textbf{post:} \quad m = \text{PEDESTRIAN\_PHASE} \land ped = \text{ACTIVE} \land t_{ped} = 0 \land \text{ped\_light\_on} \land \text{ped\_audible\_on}
$$

### 8.3 Pedestrian Phase Exit Condition

$$
\textbf{pre:} \quad m = \text{PEDESTRIAN\_PHASE} \land t_{ped} \geq T_{ped}
$$

$$
\textbf{post:} \quad m = \text{NORMAL} \land ped = \text{NONE} \land \lnot\text{ped\_light\_on} \land \lnot\text{ped\_audible\_on}
$$

### 8.4 Button Debounce

Pedestrian button inputs are debounced over a minimum period to prevent spurious activations:

$$
\text{button\_pressed} \Leftrightarrow \text{consecutive\_active\_samples} \geq N_{debounce}
$$

Where $N_{debounce}$ is a design constant (recommended: 3 samples at $T_{poll}$ = 300ms of sustained press).

### 8.5 Pedestrian Priority Bound

A pending pedestrian request must be served within a bounded number of traffic cycles to prevent indefinite waiting:

$$
\square\;\Big(ped = \text{PENDING} \Rightarrow \lozenge_{\leq 2 \cdot (T_{green} + 2 \cdot T_{inter})}\;(m = \text{PEDESTRIAN\_PHASE})\Big)
$$

*A pedestrian request will be served within at most two full traffic cycles (~63 seconds worst case).*

---

## 9. High-Integrity C Implementation Constraints

The implementation shall follow a restricted subset of C appropriate for high-integrity safety-critical systems, drawing from MISRA C:2012 and related standards.

### 9.1 Prohibited Language Features

| Feature | Rationale |
|---|---|
| Dynamic memory allocation (`malloc`, `calloc`, `realloc`, `free`) | Unbounded resource usage, fragmentation, undefined behaviour on failure |
| Pointer arithmetic | Error-prone, difficult to verify, source of buffer overflows |
| Pointer dereference (general) | Null dereference risk; use array indexing and struct access instead |
| Recursion | Unbounded stack growth; all call depths must be statically determinable |
| `goto` statements | Unstructured control flow hinders formal reasoning |
| Variable-length arrays (VLAs) | Stack overflow risk; size not known at compile time |
| `setjmp` / `longjmp` | Non-local control flow defeats static analysis |
| Floating-point arithmetic | Rounding errors, non-determinism across platforms; use integer milliseconds for all timing |
| Union types | Type-punning risks undefined behaviour |
| Implicit type conversions (narrowing) | Silent data loss |
| Standard library I/O (`printf`, `scanf`, etc.) | Unbounded, format-string vulnerabilities; use platform HAL for output |

### 9.2 Required Practices

| Practice | Description |
|---|---|
| Static allocation only | All variables declared at file or function scope with known, bounded sizes |
| Bounded loops | Every loop must have a provable upper bound on iterations |
| Enum for state encoding | All state variables use named enumerations, never raw integers |
| Defensive default cases | Every `switch` must have a `default` that triggers a fault |
| Assertions for invariants | Runtime assertion checks on all safety invariants at every state transition |
| Single entry / single exit | Functions have one return path to simplify reasoning |
| Const correctness | Data that must not change is declared `const` |
| Fixed-width integers | Use `uint8_t`, `uint16_t`, `uint32_t` (from `<stdint.h>`) for all integer variables |
| No implicit fallthrough | Every `case` in a `switch` ends with `break` or the function returns |
| Hardware abstraction layer (HAL) | All hardware access via a defined HAL interface, enabling testing with stubs |
| Watchdog timer integration | Main loop must kick a hardware watchdog; failure to execute within deadline triggers hardware reset |

### 9.3 Code Structure Plan

```
traffic_light_system/
├── main.c                  -- Entry point, main loop, watchdog
├── types.h                 -- Enumerations, state structs, constants
├── state_machine.h/.c      -- Signal state transitions, validation
├── controller.h/.c         -- Master cycle logic, timing
├── fault_handler.h/.c      -- Fault detection, response, alerting
├── pedestrian.h/.c         -- Pedestrian subsystem logic
├── sensor.h/.c             -- Traffic sensor reading, health monitoring
├── hal.h/.c                -- Hardware abstraction layer interface
├── hal_stub.h/.c           -- Test stubs for HAL (used during verification)
├── monitor.h/.c            -- Runtime invariant checking, logging
└── tests/
    ├── test_state_machine.c
    ├── test_controller.c
    ├── test_fault_handler.c
    ├── test_invariants.c
    └── test_pedestrian.c
```

### 9.4 Verification Strategy

| Technique | Coverage |
|---|---|
| Static analysis (e.g. Polyspace, PC-lint) | MISRA rule compliance, unreachable code, uninitialised variables, overflow |
| Unit testing | Each module tested in isolation with HAL stubs |
| Invariant testing | All 8 safety invariants checked across exhaustive state-pair combinations |
| Fault injection testing | Simulated sensor failures, light failures, timing overruns |
| Model checking (optional) | SPIN/NuSMV verification of LTL properties against a model |
| Code review | Independent review of all safety-critical paths |

---

## 10. Traceability Matrix

Every requirement from the original brief is traced to a formal invariant/property and to a planned implementation module.

| Requirement (Brief) | Formal Property | Implementation Module |
|---|---|---|
| Red → Red+Amber → Green → Amber → Red cycle | §3.1 next function, INV-6 | `state_machine.c` |
| Paired installations operate synchronously | INV-3 | `controller.c`, `monitor.c` |
| Intermediate signals held for 1.5s | §6.3 Bounded Response | `controller.c` |
| Green held for 30s with waiting traffic | §6.4 Bounded Response | `controller.c` |
| Green held indefinitely if no opposing traffic | §4.2.5 Hold Condition | `controller.c` |
| Pedestrian hold for 15s on all-red | §8, §6.5 | `pedestrian.c` |
| Pedestrian alert + light during crossing | INV-8 | `pedestrian.c` |
| No conflicting permissive signals | INV-1 | `state_machine.c`, `monitor.c` |
| No traffic while pedestrians crossing | INV-2 | `pedestrian.c`, `monitor.c` |
| Sensor failure → timed mode + alert | §7.2 (F1), INV-7 | `sensor.c`, `fault_handler.c` |
| Signal fails to advance → all OFF + alert | §7.2 (F2), INV-4 | `fault_handler.c` |
| Light fails to illuminate → all OFF + alert | §7.2 (F3), INV-4 | `fault_handler.c` |
| Light fails to de-illuminate → all OFF + alert | §7.2 (F4), INV-4 | `fault_handler.c` |

---

## Appendix A: State Transition Diagram (Textual)

```
                    ┌──────────────────────────────────────────────────┐
                    │                                                  │
                    ▼                                                  │
            ┌──────────────┐   pedestrian    ┌────────────────────┐   │
     ┌─────►│  ALL RED      │──pending?──────►│  PEDESTRIAN PHASE  │───┘
     │      │  (σA=R, σB=R) │◄───────────────│  (15s, alert on)   │
     │      └──────┬────────┘   T_ped done    └────────────────────┘
     │             │ no ped
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR A:       │
     │      │ RED_AMBER     │  1.5s
     │      │ (σB=RED)      │
     │      └──────┬────────┘
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR A:       │
     │      │ GREEN         │  30s / indefinite
     │      │ (σB=RED)      │
     │      └──────┬────────┘
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR A:       │
     │      │ AMBER         │  1.5s
     │      │ (σB=RED)      │
     │      └──────┬────────┘
     │             ▼
     │      ┌──────────────┐   pedestrian    ┌────────────────────┐
     │      │  ALL RED      │──pending?──────►│  PEDESTRIAN PHASE  │
     │      │  (σA=R, σB=R) │◄───────────────│  (15s, alert on)   │
     │      └──────┬────────┘   T_ped done    └────────────────────┘
     │             │ no ped
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR B:       │
     │      │ RED_AMBER     │  1.5s
     │      │ (σA=RED)      │
     │      └──────┬────────┘
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR B:       │
     │      │ GREEN         │  30s / indefinite
     │      │ (σA=RED)      │
     │      └──────┬────────┘
     │             ▼
     │      ┌──────────────┐
     │      │ PAIR B:       │
     │      │ AMBER         │  1.5s
     │      │ (σA=RED)      │
     │      └──────┬────────┘
     │             │
     └─────────────┘

     At ANY point: fault detected ──► ALL OFF + ALERT
```

## Appendix B: Glossary

| Term | Definition |
|---|---|
| Installation | A single traffic light unit (red, amber, green lights + sensor + pedestrian subsystem) |
| Pair | Two installations on opposing lanes of the same road, operating synchronously |
| Permissive signal | A signal that allows or implies permission for traffic to move (GREEN, AMBER, RED_AMBER) |
| Intermediate signal | A transitional signal of short duration (RED_AMBER, AMBER) |
| HAL | Hardware Abstraction Layer — software interface isolating hardware-specific operations |
| Command–confirm | Pattern where every hardware write is followed by a read-back to verify execution |
| Heartbeat | Periodic status message from a sensor indicating it is functioning |
