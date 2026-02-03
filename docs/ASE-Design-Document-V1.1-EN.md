# Admissible Step Engine (ASE)
## Design Document v1.1

---

## 1. Design Scope and Relationship to Specification

This document defines the **design-level structure**
of the Admissible Step Engine (ASE).

The purpose of this Design Document is to describe **how**
the requirements defined in the **ASE Normative Specification v1.1**
are realized architecturally, without redefining or extending them.

This document is **non-normative**.
All normative requirements, guarantees, and prohibitions
are defined exclusively in the Specification.

---

## 1.1 Role of the Design Document

The Design Document serves the following purposes:

- translate normative requirements into architectural structure,
- define component boundaries and responsibilities,
- define data and control flow at the design level,
- define integration seams and API layout,
- support implementation, verification, and maintenance.

The Design Document MUST NOT:
- restate normative requirements,
- introduce new constraints or guarantees,
- weaken or reinterpret the Specification.

---

## 1.2 Relationship to Other ASE Documents

The document hierarchy is strictly defined as follows:

1. **ASE Normative Specification v1.1**  
   Defines what ASE MUST and MUST NOT do.

2. **ASE Design Document v1.1**  
   Defines how ASE is structured to satisfy the Specification.

3. **ASE Implementation Notes**  
   Define how the Design is realized in code.

In case of conflict, the Specification takes precedence.
This Design Document MUST be interpreted as subordinate to it.

---

## 1.3 Design Scope

This Design Document covers:

- architectural decomposition of ASE,
- definition of logical components,
- interaction between components,
- public API layout at the design level,
- integration boundaries with external systems.

This Design Document does NOT cover:

- admissibility semantics,
- enforcement semantics,
- numerical safety rules,
- determinism guarantees,
- error or failure handling rules.

All such topics are defined normatively
in the ASE Specification v1.1.

---

## 1.4 Design Constraints

The design of ASE is constrained by the following high-level principles,
derived directly from the Specification:

- single-step enforcement only,
- stateless operation,
- deterministic behavior,
- bounded execution,
- no ownership of state or iteration.

These constraints are assumed as given
and are not re-justified in this document.

---

## 1.5 Design Philosophy

The ASE design follows a **minimal enforcement philosophy**:

- ASE is not a system, but a component.
- ASE is not an algorithm, but a gate.
- ASE is not adaptive, but declarative.

The design emphasizes:
- clear responsibility boundaries,
- absence of hidden behavior,
- ease of inspection and certification,
- ease of integration into existing systems.

---

## 1.6 Design Audience

This document is intended for:

- system architects,
- library and SDK implementers,
- reviewers and auditors,
- integrators embedding ASE into larger systems.

It assumes familiarity with the ASE Specification v1.1,
but does not require knowledge of any underlying theory.

---

## 1.7 Section Summary

This section establishes the role, scope, and limits
of the ASE Design Document v1.1.

The Design Document exists solely to describe
the architectural realization of the Specification,
and does not define behavior independently.

---

## 2. Architectural Overview

This section describes the **high-level architecture**
of the Admissible Step Engine (ASE) and the placement of its components
within an iterative system.

The architecture presented here is derived directly
from the constraints defined in the ASE Specification v1.1,
but does not restate or reinterpret them.

---

## 2.1 Architectural Positioning

ASE is designed as a **pure enforcement component**
that is positioned between:

- a step-producing mechanism, and
- a state-updating mechanism.

At the architectural level, ASE occupies a single,
well-defined position in the control flow:

    proposed step → ASE → effective step → state update

ASE does not control the iteration loop,
the timing of step generation,
or the application of the effective step.

---

## 2.2 Architectural Boundaries

ASE defines a strict boundary between:

- **external logic**, which includes:
  - step generation,
  - domain semantics,
  - iteration control,
  - state ownership,

and

- **ASE internal logic**, which includes:
  - admissibility evaluation orchestration,
  - enforcement strategy selection,
  - step transformation coordination.

ASE does not cross this boundary in either direction.

---

## 2.3 Component-Level View

At a high level, the ASE architecture consists of the following
logical components:

1. **Enforcement Interface**
   - Entry point exposed to the integrator.
   - Receives `(State, Step)` and returns an effective `Step`.

2. **Admissibility Evaluation Adapter**
   - Connects ASE to user-provided admissibility logic.
   - Does not interpret domain semantics.

3. **Enforcement Strategy Component**
   - Coordinates the selected enforcement mode.
   - Applies transformation logic to inadmissible steps.

4. **Neutral Step Provider**
   - Supplies a neutral step when enforcement fails.
   - Defined externally, referenced internally.

These components are conceptual and MAY be realized
as functions, classes, or templates,
depending on the implementation language.

---

## 2.4 Control Flow Overview

The control flow through ASE is linear and acyclic.

At a conceptual level:

    receive input
        ↓
    invoke admissibility evaluation
        ↓
    select enforcement path
        ↓
    produce effective step
        ↓
    return to caller

There are no loops at the architectural level,
only bounded loops within specific enforcement strategies,
as defined by the implementation.

---

## 2.5 Data Flow Overview

ASE data flow is minimal and local:

- the current state is read-only,
- the proposed step is read-only,
- the effective step is the only output.

ASE does not retain references to input data
after the enforcement call completes.

---

## 2.6 Stateless Architectural Model

Architecturally, ASE is stateless.

This means:

- no component maintains persistent data,
- no component communicates with another invocation,
- no global or static mutable data is used.

State is treated as an external concern,
fully owned by the integrator.

---

## 2.7 Deployment Model

ASE is designed to be deployed as:

- a header-only library, or
- a small statically linked library.

ASE is not designed to be deployed as:

- a service,
- a daemon,
- a background process,
- a framework requiring runtime initialization.

All configuration is local to the enforcement call
or the construction of the enforcement object.

---

## 2.8 Architectural Summary

The ASE architecture is intentionally minimal:

- one entry point,
- one responsibility,
- one directional flow.

This simplicity ensures that ASE can be:
- embedded easily,
- reasoned about locally,
- audited and certified,
- reused across domains without modification.

---

## 3. Component Decomposition

This section decomposes the Admissible Step Engine (ASE)
into **logical components** and defines their responsibilities
and interaction boundaries.

The decomposition is **architectural**, not normative.
It describes how responsibilities are separated,
not additional requirements.

---

## 3.1 Enforcement Interface

The **Enforcement Interface** is the sole public entry point
of the ASE architecture.

Responsibilities:
- accept the current `State` and proposed `Step`,
- coordinate the internal enforcement flow,
- return an effective `Step` to the caller.

Characteristics:
- synchronous and blocking,
- free of side effects,
- does not own or mutate the state,
- does not persist data across calls.

The Enforcement Interface contains no domain logic.

---

## 3.2 Admissibility Evaluation Adapter

The **Admissibility Evaluation Adapter** acts as a boundary layer
between ASE and user-provided admissibility logic.

Responsibilities:
- invoke external admissibility evaluation functions,
- pass `(State, Step)` without transformation,
- return admissibility decisions to the enforcement flow.

Characteristics:
- stateless,
- purely connective,
- does not interpret results beyond pass/fail semantics.

The adapter isolates ASE from domain-specific representations.

---

## 3.3 Enforcement Strategy Component

The **Enforcement Strategy Component** encapsulates
the selected enforcement mode.

Responsibilities:
- coordinate step transformation attempts,
- manage bounded iteration within a strategy (if applicable),
- select between pass-through, modification, or neutralization.

Characteristics:
- one strategy active per ASE instance,
- no strategy switching at runtime,
- no internal memory beyond local variables.

The strategy component does not decide admissibility;
it reacts to admissibility evaluation outcomes.

---

## 3.4 Step Transformation Utilities

**Step Transformation Utilities** represent domain-provided
operations used by enforcement strategies.

Examples:
- step scaling,
- step projection,
- step copying or normalization.

Responsibilities:
- perform pure step transformations,
- preserve domain semantics as defined externally.

Characteristics:
- deterministic,
- side-effect free,
- externally defined and injected.

ASE does not assume the existence of any specific transformation.

---

## 3.5 Neutral Step Provider

The **Neutral Step Provider** supplies a neutral (no-op) step
when enforcement cannot produce an admissible modified step.

Responsibilities:
- provide a deterministic neutral step,
- guarantee that application of the neutral step
  leaves the state unchanged.

Characteristics:
- externally defined,
- referenced but not constructed by ASE,
- invoked only under enforcement failure conditions.

---

## 3.6 Configuration Component

The **Configuration Component** represents fixed parameters
that influence enforcement behavior.

Examples:
- selected enforcement mode,
- maximum iteration bounds,
- numerical tolerances.

Characteristics:
- immutable after construction,
- read-only during enforcement,
- shared across enforcement calls.

Configuration does not encode policy,
only parameters.

---

## 3.7 Component Interaction Summary

Component interactions follow a simple, acyclic pattern:

- Enforcement Interface orchestrates flow.
- Admissibility Adapter supplies decisions.
- Strategy Component coordinates transformations.
- Transformation Utilities modify steps.
- Neutral Step Provider supplies fallback steps.

No component:
- owns the state,
- persists data across calls,
- bypasses the Enforcement Interface.

---

## 3.8 Decomposition Summary

The ASE component decomposition:

- separates concerns cleanly,
- isolates domain logic from enforcement logic,
- supports multiple enforcement strategies,
- preserves statelessness and simplicity.

This decomposition directly supports
implementation clarity and auditability.

---

## 4. Control and Data Flow

This section describes the **control flow** and **data flow**
within the Admissible Step Engine (ASE) at the design level.

The purpose of this section is to clarify
how information moves through ASE components
without redefining enforcement semantics.

---

## 4.1 Control Flow Overview

Control flow through ASE is **linear and synchronous**.

At the design level, the sequence of operations is:

1. Invocation of the Enforcement Interface.
2. Delegation to the Admissibility Evaluation Adapter.
3. Selection of the enforcement path.
4. Optional invocation of step transformation utilities.
5. Fallback to neutral step if required.
6. Return of the effective step to the caller.

Control returns immediately to the caller
after the effective step is produced.

---

## 4.2 Control Flow Characteristics

The control flow is characterized by:

- single entry point,
- single exit point,
- no branching outside enforcement strategies,
- no asynchronous execution.

ASE does not introduce:
- background tasks,
- deferred execution,
- callbacks beyond user-provided functions.

---

## 4.3 Data Inputs

ASE operates on exactly two external data inputs:

- the current `State`,
- the proposed `Step`.

Data input characteristics:

- inputs are treated as read-only,
- inputs are not modified in place,
- inputs are not retained beyond the enforcement call.

ASE does not validate semantic correctness of inputs,
only their suitability for enforcement.

---

## 4.4 Internal Data Handling

Internally, ASE may create temporary representations of steps
for the purpose of enforcement.

Internal data handling rules:

- all temporaries have bounded lifetime,
- all temporaries are local to the enforcement call,
- no internal data persists across invocations.

State data is never copied or transformed by ASE
unless explicitly required by external transformation logic.

---

## 4.5 Data Outputs

ASE produces exactly one output:

- the effective `Step`.

The effective step:

- is admissible by construction,
- is the only artifact returned by ASE,
- contains no metadata about enforcement decisions.

ASE does not return:
- diagnostic information,
- enforcement traces,
- historical context.

---

## 4.6 Data Ownership

Data ownership boundaries are strict:

- the caller owns the state,
- the caller owns the proposed step,
- the caller owns the effective step after return.

ASE owns no data beyond the scope of the call.

---

## 4.7 Flow Isolation

ASE isolates control and data flow such that:

- failures are local to the current step,
- no failure propagates beyond the enforcement call,
- no cascading effects occur across iterations.

ASE does not modify:
- iteration counters,
- loop conditions,
- control variables outside enforcement.

---

## 4.8 Control and Data Flow Summary

The ASE control and data flow model is intentionally simple:

- one call in,
- one step out,
- no retained context.

This simplicity supports:
- predictability,
- ease of reasoning,
- safe integration into complex systems.

---

## 5. Public API Layout

This section describes the **design-level layout**
of the public API exposed by the Admissible Step Engine (ASE).

The API layout defines how integrators interact with ASE
without redefining behavioral or normative requirements.

---

## 5.1 API Design Goals

The public API is designed to satisfy the following goals:

- minimal surface area,
- explicit control flow,
- clear separation of responsibilities,
- ease of integration into existing codebases,
- suitability for both embedded and general-purpose use.

The API exposes enforcement capability only;
it does not expose internal components or logic.

---

## 5.2 Canonical Enforcement Entry Point

At the design level, ASE exposes a **single canonical operation**:

- accept the current `State`,
- accept a proposed `Step`,
- return an effective `Step`.

This operation represents the complete enforcement interaction
between the caller and ASE.

The API does not expose intermediate enforcement states,
partial results, or internal decisions.

---

## 5.3 Configuration Exposure

Configuration parameters influencing enforcement behavior
are provided at construction or initialization time.

Design characteristics:

- configuration is explicit,
- configuration is immutable after construction,
- configuration is read-only during enforcement.

The API does not permit:
- dynamic reconfiguration,
- runtime mode switching,
- implicit configuration via global state.

---

## 5.4 Injection of Domain Logic

Domain-specific logic is injected into ASE via the API.

Injected elements include:

- admissibility evaluation logic,
- step transformation utilities (scaling, projection),
- neutral step construction logic.

Injection characteristics:

- injection occurs explicitly,
- injected logic is treated as opaque,
- ASE does not interpret domain semantics.

This design allows ASE to remain domain-agnostic.

---

## 5.5 Error and Status Reporting

The public API MAY expose optional status reporting mechanisms,
such as return codes or status objects.

Design constraints:

- status reporting is optional,
- status reporting does not alter enforcement behavior,
- absence of status reporting does not affect safety guarantees.

The API does not expose exceptions
as part of its required interface.

---

## 5.6 API Stability Considerations

The public API is designed to be **stable across versions**.

Design principles:

- additions MAY be made in a backward-compatible manner,
- removal or semantic change of existing API elements
  requires a major version increment,
- internal refactoring MUST NOT affect the public API.

---

## 5.7 Minimal API Philosophy

The API intentionally exposes:

- what is necessary to enforce admissibility,
- nothing more.

It intentionally hides:

- internal enforcement strategies,
- internal iteration mechanics,
- internal safety checks.

This minimalism supports:
- ease of use,
- ease of auditing,
- ease of long-term maintenance.

---

## 5.8 API Layout Summary

The ASE public API is:

- small,
- explicit,
- deterministic,
- domain-agnostic.

It provides a single, well-defined integration point
for enforcing admissible steps in iterative systems.

---

## 6. Configuration and Profiles (Reference Only)

This section describes the **design-level treatment**
of configuration parameters and implementation profiles
used by the Admissible Step Engine (ASE).

This section is **descriptive**, not normative.
All normative requirements are defined in the Specification.

---

## 6.1 Configuration Role in Design

Configuration in ASE serves to:

- select the enforcement strategy,
- define fixed bounds for enforcement behavior,
- parameterize numerical handling.

At the design level, configuration is treated as
**static context** supplied to the enforcement logic.

Configuration does not encode policy or decision-making logic;
it supplies parameters only.

---

## 6.2 Configuration Characteristics

Design characteristics of ASE configuration:

- configuration is explicit,
- configuration is immutable after initialization,
- configuration is shared read-only across enforcement calls.

Configuration is not part of the dynamic control flow
and does not evolve during execution.

---

## 6.3 Typical Configuration Parameters

At the design level, configuration MAY include:

- selected enforcement mode,
- maximum iteration limits for strategies,
- numerical tolerance parameters,
- minimum or maximum scaling factors.

The specific set of parameters is implementation-dependent,
but all parameters are treated uniformly as fixed inputs.

---

## 6.4 Profiles as Design Context

Implementation profiles provide context
for how ASE is expected to be realized.

Profiles influence:

- memory usage strategies,
- error signaling mechanisms,
- availability of certain language features.

Profiles do not alter:
- enforcement semantics,
- admissibility criteria,
- control or data flow.

---

## 6.5 Profile Awareness in Design

From a design perspective:

- profiles constrain implementation choices,
- profiles do not introduce conditional behavior in design,
- the same architectural structure applies across profiles.

Design artifacts remain identical regardless of profile;
only realization details differ.

---

## 6.6 Configuration and Profiles Summary

Configuration and profiles:

- parameterize behavior without altering structure,
- constrain implementation without changing architecture,
- preserve uniformity across domains and environments.

At the design level, they are treated as
external, fixed context for enforcement.

---

## 7. Integration Boundaries

This section defines the **integration boundaries**
between the Admissible Step Engine (ASE)
and the systems into which it is embedded.

The goal of this section is to make explicit
what ASE assumes about its environment
and what it deliberately does not assume.

---

## 7.1 External System Responsibilities

At the design level, the external system is responsible for:

- owning and managing the system state,
- generating proposed steps,
- applying effective steps to the state,
- controlling iteration flow and scheduling,
- defining domain semantics and correctness.

ASE does not replace or interfere
with any of these responsibilities.

---

## 7.2 ASE Responsibilities at the Boundary

At the integration boundary, ASE is responsible for:

- accepting the current state and proposed step,
- enforcing admissibility on the proposed step,
- returning an effective step to the caller.

ASE does not cross this boundary in either direction.

---

## 7.3 No Assumptions About Iteration Context

ASE makes no assumptions about:

- iteration frequency,
- iteration ordering,
- synchronous or asynchronous execution,
- real-time or offline operation.

ASE treats each enforcement invocation
as an isolated interaction.

---

## 7.4 No Assumptions About State Representation

ASE does not assume:

- a particular data structure for the state,
- a particular memory layout,
- a particular ownership or lifetime model.

The state is treated as an opaque entity
that is passed through the API.

---

## 7.5 No Assumptions About Step Semantics

ASE does not assume:

- how a step was produced,
- what a step represents semantically,
- whether a step is additive, multiplicative, or symbolic.

ASE interacts with steps only
through injected domain logic.

---

## 7.6 Language and Runtime Independence

At the design level, ASE is:

- independent of programming language specifics,
- independent of runtime frameworks,
- independent of operating system services.

These constraints ensure that ASE
can be implemented consistently
across environments.

---

## 7.7 Integration Anti-Patterns

The following integration patterns are discouraged
at the design level:

- embedding ASE inside state objects,
- allowing ASE to modify global system state,
- coupling ASE to logging or monitoring frameworks,
- using ASE as a controller or decision-maker.

Such patterns violate the intended separation of concerns.

---

## 7.8 Integration Boundaries Summary

ASE defines a **hard, minimal integration boundary**:

- inputs in: `(State, Step)`,
- output out: `Effective Step`.

Everything else remains
the responsibility of the surrounding system.

This boundary is essential for
predictable behavior and safe integration.

---

## 8. Non-Normative Design Rationale

This section provides **design rationale**
for the architectural choices made in ASE.

This section is **informative**.
It does not introduce requirements
and does not alter the Specification.

---

## 8.1 Minimal Surface Area

ASE is designed with a deliberately minimal surface area
to reduce integration risk and cognitive overhead.

A small API:
- reduces misuse,
- simplifies verification,
- supports long-term stability.

The design favors fewer concepts
over expressive but complex abstractions.

---

## 8.2 Stateless Architecture

Statelessness was chosen to ensure:

- deterministic behavior,
- ease of reasoning,
- trivial thread safety,
- suitability for safety-critical systems.

By eliminating internal memory,
ASE avoids entire classes of failure modes.

---

## 8.3 Single-Step Focus

ASE operates exclusively on individual steps
rather than sequences or trajectories.

This focus allows:
- local enforcement,
- bounded execution,
- applicability in real-time contexts.

Multi-step reasoning is deliberately excluded
to avoid complexity and hidden dynamics.

---

## 8.4 Separation of Enforcement and Semantics

ASE separates enforcement mechanics
from domain semantics.

This separation ensures that:
- ASE remains domain-agnostic,
- domain logic remains external and explicit,
- enforcement behavior is predictable.

ASE enforces constraints
without interpreting meaning.

---

## 8.5 Neutral Step as Safe Fallback

The use of a neutral step as a fallback outcome
ensures fail-safe behavior
without requiring rollback or recovery logic.

This choice simplifies integration
and avoids coupling ASE to system-level error handling.

---

## 8.6 Configuration Immutability

Immutable configuration prevents:
- runtime drift,
- adaptive behavior,
- configuration-induced non-determinism.

This design choice supports auditability
and repeatable behavior.

---

## 8.7 Library-Oriented Deployment

Deploying ASE as a library rather than a service
avoids operational complexity
and allows enforcement to occur inline.

This choice reflects ASE’s role
as a low-level enforcement primitive.

---

## 8.8 Rationale Summary

The ASE design favors:

- simplicity over expressiveness,
- predictability over flexibility,
- enforcement over optimization.

These choices align ASE with its intended role
as a universal admissibility enforcement mechanism.

---

## 9. Design Summary

This section summarizes the **architectural design**
of the Admissible Step Engine (ASE).

The summary consolidates the design intent
without introducing new concepts or requirements.

---

## 9.1 Design Intent

ASE is designed as a **minimal, stateless enforcement component**
that ensures only admissible steps are executed
in iterative systems.

The design intentionally avoids:
- ownership of system state,
- control over iteration flow,
- interpretation of domain semantics.

---

## 9.2 Architectural Characteristics

Key architectural characteristics of ASE include:

- single, explicit entry point,
- linear and bounded control flow,
- strict separation of responsibilities,
- no persistent internal state,
- deterministic interaction model.

These characteristics support
predictability and ease of verification.

---

## 9.3 Integration Characteristics

ASE integrates as a **library-level component**
with a clearly defined boundary:

- inputs: current state and proposed step,
- output: effective step.

ASE does not introduce
framework dependencies or operational overhead.

---

## 9.4 Relationship to Specification

The design described in this document
is a direct architectural realization
of the requirements defined in the
ASE Normative Specification v1.1.

The Design Document does not modify,
extend, or reinterpret the Specification.

---

## 9.5 Design Completeness

With this section, the ASE Design Document v1.1
fully describes the architectural structure
required to implement ASE.

All remaining aspects of ASE realization
are addressed in the Implementation Notes
and verification artifacts.

---

## 9.6 End of Design Document

This section concludes the
ASE Design Document v1.1.
