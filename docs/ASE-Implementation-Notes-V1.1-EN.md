# Admissible Step Engine (ASE)
## Implementation Notes v1.1

---

## 1. Purpose and Status of This Document

This document describes the **implementation-level realization**
of the Admissible Step Engine (ASE).

It is written to support:
- consistent implementation across environments,
- mechanical translation of design into code,
- verification and maintenance.

This document is **prescriptive but non-normative**.

All behavioral requirements, guarantees, and prohibitions
are defined exclusively in:

- ASE Normative Specification v1.1
- ASE Design Document v1.1

Nothing in this document may weaken, extend,
or reinterpret those documents.

---

## 1.1 Role in the ASE Document Hierarchy

The ASE documentation hierarchy is:

1. **ASE Normative Specification v1.1**  
   Defines required external behavior.

2. **ASE Design Document v1.1**  
   Defines architectural structure and boundaries.

3. **ASE Implementation Notes v1.1**  
   Describe how the design is realized in code.

These Implementation Notes:
- do not introduce new requirements,
- do not redefine safety or determinism rules,
- do not serve as a compliance reference by themselves.

---

## 1.2 Scope of Implementation Notes

This document covers:

- implementation flow corresponding to design components,
- algorithmic structure of enforcement logic,
- handling of edge cases at the code level,
- constraints relevant to C++ implementations.

This document does not cover:

- admissibility semantics,
- enforcement correctness criteria,
- compliance definitions,
- certification requirements.

All such aspects are defined in the Specification.

---

## 1.3 Implementation Philosophy

The implementation of ASE is guided by the following principles:

- **Direct realization**  
  Each implementation element maps directly to a design component.

- **Mechanical translation**  
  Code should be writable without interpretation or inference.

- **Minimalism**  
  No functionality beyond what is required by the design.

- **Transparency**  
  Implementation behavior should be obvious from structure.

The goal is not cleverness,
but predictability and inspectability.

---

## 1.4 Language and Environment Assumptions

These notes assume an implementation in:

- modern C++ (C++17 or later),
- library form (header-only or static library),
- without mandatory external dependencies.

Other languages may follow analogous structure,
but this document focuses on C++ realization.

---

## 1.5 Implementation Boundary

At the implementation level:

- ASE does not own system state,
- ASE does not control iteration flow,
- ASE does not interpret domain semantics.

Implementation code must respect these boundaries strictly.
Any deviation constitutes an implementation error.

---

## 1.6 Section Summary

This section establishes the **status, scope, and limits**
of the ASE Implementation Notes v1.1.

The sections that follow describe how the
ASE Design Document v1.1 is realized in code,
without redefining behavior.

---

## 2. Implementation Scope and Structure

This section describes the **overall scope**
and **structural organization** of an ASE implementation.

The intent is to translate the architectural design
into a concrete and unambiguous code structure.

---

## 2.1 Implementation Scope

An ASE implementation consists of:

- a single enforcement entry point,
- supporting internal functions corresponding to design components,
- externally injected domain-specific logic.

The implementation does not include:

- system state management,
- iteration control logic,
- domain-specific decision-making.

All such responsibilities remain external to ASE.

---

## 2.2 Logical Module Structure

At the code level, ASE may be organized into the following
logical modules or sections:

- **enforcement interface**
- **admissibility invocation**
- **strategy coordination**
- **step transformation invocation**
- **neutral step handling**
- **configuration access**

These modules may be realized as:
- functions,
- classes,
- templates,
- or combinations thereof.

The choice of realization does not affect external behavior.

---

## 2.3 Mapping to Design Components

Each logical module maps directly to a design component:

- enforcement interface → Enforcement Interface
- admissibility invocation → Admissibility Evaluation Adapter
- strategy coordination → Enforcement Strategy Component
- step transformation invocation → Step Transformation Utilities
- neutral step handling → Neutral Step Provider
- configuration access → Configuration Component

This mapping is one-to-one and intentional.

---

## 2.4 Single Entry Point Structure

The implementation exposes a single public operation
corresponding to the enforcement interface.

Internally, this operation orchestrates:

- admissibility evaluation,
- enforcement strategy selection,
- step transformation attempts,
- fallback to neutral step.

The public entry point contains no domain logic.

---

## 2.5 Internal Function Boundaries

Internal helper functions:

- operate only on parameters passed explicitly,
- do not access global or static mutable state,
- do not retain data between calls.

Function boundaries are chosen to:
- reflect design responsibilities,
- simplify reasoning and testing.

---

## 2.6 Configuration Access Pattern

Configuration is accessed as a read-only context.

At the implementation level:

- configuration is passed explicitly,
- configuration values are not modified,
- configuration values are not recomputed dynamically.

This pattern ensures predictable behavior.

---

## 2.7 Implementation Structure Summary

The ASE implementation structure is:

- minimal,
- modular,
- directly traceable to design elements.

This structure enables straightforward
verification and maintenance.

---

## 3. Core Enforcement Execution Flow

This section describes the **implementation-level execution flow**
of the ASE enforcement operation.

The flow described here is a direct realization
of the architectural control flow defined
in the ASE Design Document v1.1.

---

## 3.1 High-Level Execution Sequence

At the implementation level, enforcement proceeds as a
single, synchronous operation.

The conceptual sequence is:

    receive (State S, Step dS)
        ↓
    evaluate admissibility of dS
        ↓
    if admissible:
        return dS
    else:
        invoke enforcement strategy
        ↓
    if strategy yields admissible step:
        return transformed step
    else:
        return neutral step

The implementation follows this sequence directly,
without reordering or shortcutting stages.

---

## 3.2 Enforcement Entry Function

The enforcement entry function:

- receives the current state and proposed step,
- orchestrates the full enforcement process,
- returns the effective step.

The entry function does not:
- apply the step to the state,
- modify external data,
- perform validation beyond what is required
  to safely invoke enforcement logic.

---

## 3.3 Admissibility Invocation

Admissibility evaluation is invoked as an explicit,
isolated call within the enforcement flow.

At the implementation level:

- admissibility is evaluated before any transformation,
- admissibility is re-evaluated for transformed steps,
- no admissibility result is cached or reused.

The implementation treats admissibility evaluation
as an opaque boolean decision.

---

## 3.4 Strategy Dispatch

If the proposed step is not admissible,
control is dispatched to the selected enforcement strategy.

Strategy dispatch:

- is explicit and static,
- does not involve dynamic selection logic,
- does not depend on runtime history.

Only one strategy is active for a given ASE instance.

---

## 3.5 Strategy Execution

Within a strategy, the implementation may:

- invoke step transformation utilities,
- evaluate admissibility of transformed steps,
- terminate early upon success.

Strategy execution is:

- bounded,
- deterministic,
- local to the current enforcement call.

No strategy accumulates information across calls.

---

## 3.6 Neutral Step Fallback

If strategy execution does not yield
an admissible transformed step,
the implementation falls back to the neutral step.

Neutral step selection:

- is explicit,
- does not involve inference,
- does not depend on failure type.

The neutral step is returned as the effective step.

---

## 3.7 Early Exit Conditions

The implementation exits enforcement immediately when:

- the proposed step is admissible,
- a transformed admissible step is found.

No further processing occurs after an effective step
has been selected.

---

## 3.8 Error Containment

Any error encountered during enforcement
is contained within the enforcement operation.

At the implementation level:

- errors do not propagate outward,
- the enforcement function always returns a step,
- fallback behavior is local to the call.

This containment simplifies integration.

---

## 3.9 Execution Flow Summary

The ASE enforcement execution flow is:

- linear,
- bounded,
- deterministic,
- isolated.

The implementation mirrors the design flow exactly,
enabling mechanical verification and reasoning.

---

## 4. Admissibility Evaluation Handling (Implementation-Level)

This section describes how admissibility evaluation
is handled at the implementation level.

The intent is to ensure correct invocation,
isolation, and reuse of admissibility logic
without redefining admissibility semantics.

---

## 4.1 Invocation Pattern

Admissibility evaluation is invoked explicitly
as a function call supplied by the integrator.

At the implementation level:

- the evaluation function is called with `(State, Step)`,
- the result is treated as a boolean decision,
- no interpretation beyond pass/fail is performed.

The implementation does not attempt to infer
reasons or categories of inadmissibility.

---

## 4.2 Evaluation Timing

Admissibility evaluation occurs at well-defined points:

- once for the proposed step,
- once for each transformed step produced by a strategy.

Evaluation does not occur:

- before inputs are available,
- after an effective step has been selected,
- outside the enforcement operation.

This ensures consistent and predictable behavior.

---

## 4.3 Isolation of Evaluation Logic

At the implementation level, admissibility logic is isolated:

- it is not embedded in enforcement code,
- it is not interleaved with strategy logic,
- it is not modified or wrapped implicitly.

This isolation allows admissibility logic
to be tested independently of enforcement.

---

## 4.4 Handling of Evaluation Failures

If admissibility evaluation fails to produce
a valid decision at runtime
(e.g., due to an exception or invalid computation):

- the implementation treats the step as inadmissible,
- control proceeds to enforcement fallback logic.

Failure handling is local to the current call
and does not affect subsequent invocations.

---

## 4.5 Reuse Across Strategies

The same admissibility evaluation function
is reused across all enforcement strategies.

The implementation does not:

- substitute different evaluation logic per strategy,
- alter evaluation parameters dynamically,
- maintain strategy-specific admissibility state.

This ensures uniform enforcement behavior.

---

## 4.6 No Caching or Memoization

The implementation does not cache
admissibility evaluation results.

Each invocation:

- evaluates admissibility fresh,
- depends only on current inputs.

This avoids hidden coupling between steps.

---

## 4.7 Evaluation Handling Summary

At the implementation level, admissibility evaluation is:

- explicit,
- isolated,
- repeatable,
- treated as an opaque decision source.

The implementation coordinates evaluation
without interpreting its semantics.

---

## 5. Enforcement Strategy Implementation

This section describes the **implementation-level realization**
of enforcement strategies used by the Admissible Step Engine (ASE).

The descriptions here explain how strategies are structured
and invoked in code, without redefining enforcement semantics.

---

## 5.1 Strategy Selection in Implementation

At the implementation level, the enforcement strategy:

- is selected during construction or initialization,
- is stored as an immutable configuration choice,
- is not changed during runtime.

Strategy selection is resolved once
and does not involve conditional logic per call.

---

## 5.2 Reject Strategy Implementation

In the Reject strategy implementation:

- admissibility of the proposed step is evaluated,
- if admissible, the step is returned immediately,
- if inadmissible, the neutral step is returned.

The implementation contains no loops
and no step transformations in this strategy.

This strategy serves as the simplest realization
of the enforcement flow.

---

## 5.3 Scale Strategy Implementation

In the Scale strategy implementation:

- a scaling factor is initialized from configuration,
- the proposed step is transformed using scaling logic,
- admissibility of the scaled step is evaluated,
- scaling proceeds iteratively within configured bounds.

Implementation characteristics:

- iteration bounds are explicit,
- scaling factor progression is monotonic,
- intermediate scaled steps are not persisted.

If no admissible scaled step is found,
the implementation returns the neutral step.

---

## 5.4 Project Strategy Implementation

In the Project strategy implementation:

- the proposed step is passed to projection logic,
- a single projected step is produced,
- admissibility of the projected step is evaluated.

The implementation:

- performs projection at most once,
- does not iterate or refine projection results,
- does not combine projection with scaling.

If the projected step is not admissible,
the neutral step is returned.

---

## 5.5 Strategy Execution Boundaries

Across all strategies, the implementation ensures that:

- execution is local to the enforcement call,
- no strategy retains internal state,
- no strategy modifies configuration parameters.

Strategies do not communicate with each other
and share no mutable data.

---

## 5.6 Strategy Failure Handling

If a strategy encounters an internal failure
(e.g., inability to produce a transformed step):

- the failure is contained within the strategy,
- the strategy returns control to the enforcement flow,
- the enforcement flow falls back to the neutral step.

The implementation does not attempt recovery
beyond the current enforcement call.

---

## 5.7 Strategy Implementation Summary

At the implementation level, enforcement strategies are:

- structurally simple,
- bounded in execution,
- deterministic by construction.

Each strategy is a direct realization
of its corresponding design component.

---

## 6. Neutral Step Handling (Implementation-Level)

This section describes how the **neutral (no-op) step**
is handled at the implementation level within ASE.

The neutral step provides the implementation-level
fallback mechanism when enforcement cannot yield
an admissible transformed step.

---

## 6.1 Neutral Step Provision

At the implementation level, the neutral step is:

- supplied externally by the integrator,
- injected into ASE during construction or initialization,
- referenced but not constructed by ASE itself.

The implementation does not attempt to derive,
infer, or synthesize a neutral step.

---

## 6.2 Neutral Step Invocation

The neutral step is selected as the effective step
in the following implementation scenarios:

- the proposed step is inadmissible
  and no strategy produces an admissible alternative,
- a strategy encounters an internal failure,
- admissibility evaluation fails at runtime.

Neutral step selection is explicit
and occurs at a single, well-defined point
in the enforcement flow.

---

## 6.3 Neutral Step Usage Characteristics

At the implementation level, the neutral step:

- is returned as-is,
- is not transformed or modified,
- is not wrapped with metadata or annotations.

The implementation treats the neutral step
as an opaque value.

---

## 6.4 Neutral Step Lifetime and Ownership

The implementation does not assume ownership
of the neutral step beyond the scope of a call.

Specifically:

- the neutral step is not stored persistently,
- references to the neutral step are not retained,
- the neutral step is treated identically to any other step value.

Ownership and lifetime are managed externally.

---

## 6.5 Neutral Step and Error Containment

Use of the neutral step ensures that:

- enforcement failures remain local,
- no exception or failure propagates outward,
- the enforcement function always returns a valid step.

This simplifies error containment
and integration with surrounding systems.

---

## 6.6 Neutral Step Handling Summary

At the implementation level, neutral step handling is:

- explicit,
- simple,
- non-intrusive.

The neutral step serves as the universal fallback
without introducing additional control logic.

---

## 7. Configuration Handling (Implementation-Level)

This section describes how configuration data
is handled within an ASE implementation.

Configuration handling at the implementation level
realizes the design assumptions
without introducing dynamic behavior.

---

## 7.1 Configuration Injection

Configuration parameters are supplied to ASE:

- at construction time, or
- during explicit initialization.

At the implementation level:

- configuration is passed explicitly,
- configuration is not inferred or discovered,
- configuration is not modified after initialization.

---

## 7.2 Configuration Representation

Configuration may be represented as:

- a simple struct,
- a lightweight object,
- a compile-time parameter set.

Regardless of representation, configuration values are:

- immutable during enforcement,
- accessed in a read-only manner,
- shared safely across calls if needed.

---

## 7.3 Configuration Access Pattern

During enforcement execution:

- configuration values are read directly,
- no caching or recomputation occurs,
- no conditional modification is performed.

Configuration access is local and predictable,
supporting deterministic execution.

---

## 7.4 Configuration Scope

Configuration influences:

- enforcement strategy selection,
- iteration bounds for strategies,
- numerical tolerance values.

Configuration does not influence:

- admissibility semantics,
- state interpretation,
- control flow outside enforcement.

---

## 7.5 Profile-Related Configuration

Implementation profiles may constrain
how configuration is represented or accessed.

Examples:

- profile-specific limits on allocation,
- profile-specific error signaling mechanisms.

Such constraints affect implementation choices
but do not alter enforcement behavior.

---

## 7.6 Configuration Handling Summary

At the implementation level, configuration handling is:

- explicit,
- immutable,
- non-adaptive.

Configuration provides fixed context
for enforcement execution
without introducing hidden dynamics.

---

## 8. Error Containment and Failure Handling (Implementation-Level)

This section describes how errors and failures
are handled at the implementation level within ASE.

The purpose of this section is to explain
how failure containment is realized in code,
without redefining failure semantics.

---

## 8.1 Error Containment Principle

At the implementation level, all errors are **contained**
within the enforcement operation.

This means:

- the enforcement function always returns a step,
- errors do not propagate to the caller,
- enforcement does not terminate the program.

Error containment is achieved structurally,
not through external recovery mechanisms.

---

## 8.2 Handling of Runtime Failures

Runtime failures that may occur during enforcement include:

- failure in admissibility evaluation invocation,
- failure in step transformation logic,
- unexpected computation issues.

At the implementation level, such failures are handled by:

- aborting the current enforcement path,
- selecting the neutral step as the effective step,
- returning control to the caller.

---

## 8.3 Exception Containment

In implementations where exceptions are used internally:

- exceptions are caught within enforcement boundaries,
- exceptions do not escape the enforcement function,
- exception handling paths converge to neutral step fallback.

Exception handling does not alter
the observable enforcement flow.

---

## 8.4 Error Signaling to the Caller

The implementation MAY optionally expose
error or status signaling mechanisms.

Such mechanisms:

- are explicit and documented,
- do not alter enforcement behavior,
- do not influence the effective step returned.

Error signaling is informational only.

---

## 8.5 No Side Effects on Failure

At the implementation level, failure handling:

- does not modify external state,
- does not trigger logging or I/O,
- does not alter configuration or internal data.

Failure effects are local to the enforcement call.

---

## 8.6 Failure Handling Summary

Implementation-level failure handling in ASE:

- is localized,
- is predictable,
- does not interfere with surrounding systems.

This approach supports safe embedding
into complex execution environments.

---

## 9. Determinism and Boundedness in Implementation

This section describes how **determinism and boundedness**
are realized at the implementation level in ASE.

The intent is to explain structural and coding practices
that support these properties,
without redefining their normative meaning.

---

## 9.1 Deterministic Control Flow

At the implementation level, determinism is achieved by:

- explicit control flow without data-dependent branching,
- fixed strategy selection,
- explicit ordering of operations.

The enforcement function executes
the same sequence of operations
for identical inputs and configuration.

---

## 9.2 Deterministic Use of External Logic

External logic injected into ASE
(admissibility evaluation, transformations)
is invoked in a fixed, explicit order.

The implementation:

- does not reorder calls,
- does not parallelize execution,
- does not short-circuit evaluation beyond defined exits.

This preserves determinism at the integration boundary.

---

## 9.3 Bounded Iteration in Strategies

Where strategies involve iteration
(e.g., scaling loops):

- iteration bounds are explicit,
- bounds are derived from configuration,
- no unbounded loops are present.

Iteration terminates either upon success
or upon reaching the configured limit.

---

## 9.4 Memory Usage Characteristics

At the implementation level:

- memory usage per enforcement call is bounded,
- no memory is accumulated across calls,
- temporary objects have limited lifetime.

In constrained profiles,
dynamic allocation may be avoided entirely.

---

## 9.5 No Recursion or Unbounded Calls

The implementation avoids:

- recursion,
- indirect unbounded call chains,
- dynamic dispatch that could introduce variability.

All call paths are visible and bounded.

---

## 9.6 Determinism and Boundedness Summary

Through explicit structure and constraints,
the implementation ensures that:

- execution is repeatable,
- execution time is bounded,
- resource usage is predictable.

These properties align the implementation
with the design intent and specification.

---

## 10. Implementation Summary

This section summarizes the **implementation-level realization**
of the Admissible Step Engine (ASE).

The summary consolidates implementation intent
without introducing new behavior or requirements.

---

## 10.1 Implementation Role

The ASE implementation realizes the enforcement behavior
defined in the Specification
using the architectural structure defined in the Design Document.

It provides a concrete, inspectable realization
that can be implemented mechanically in code.

---

## 10.2 Structural Characteristics

At the implementation level, ASE is characterized by:

- a single enforcement entry point,
- linear and explicit control flow,
- bounded execution paths,
- absence of persistent internal state,
- explicit handling of failure and fallback.

These characteristics simplify verification
and support safe integration.

---

## 10.3 Integration Characteristics

The implementation integrates as:

- a library component,
- embedded directly in iteration loops,
- independent of runtime frameworks.

It does not impose requirements
on state representation or system architecture.

---

## 10.4 Alignment with Specification and Design

The implementation described in this document:

- aligns with ASE Normative Specification v1.1,
- realizes the ASE Design Document v1.1 directly,
- does not introduce additional semantics.

Any compliant implementation
must adhere to the Specification;
this document provides guidance on how to do so.

---

## 10.5 Implementation Completeness

With this section, the ASE Implementation Notes v1.1
fully describe how ASE is realized in code.

Further artifacts, such as:
- reference implementations,
- test suites,
- compliance matrices,

may be developed based on these notes.

---

## 10.6 End of Implementation Notes

This section concludes the
ASE Implementation Notes v1.1.
