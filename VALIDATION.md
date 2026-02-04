## Learning-Loop Envelope Simulations (Internal)

These simulations validate ASE as a fail-closed safety envelope over training dynamics,
enforcing constraints on the next optimizer state rather than on raw gradients.

Two internal simulations demonstrate applicability of ASE as a safety envelope for ML-style training loops.
ASE is applied as a gate over a proposed optimizer update ΔS, producing ΔS_eff such that the next state remains admissible.

### A) Parameter-only loop (theta)
- State: θ ∈ R^N
- Proposed step: Δθ (SGD-like with noise, rare spikes, rare NaN/Inf injections)
- Admissibility: θ_next finite, ||θ_next||_2 ≤ R, and sign(θ_next[i]) ≥ 0 for i < K
- Result: baseline without ASE corrupts state (NaN/Inf); ASE Scale/Project keeps θ within bounds and fail-closes to neutral on invalid proposals.
- Test: `ASE_LearningEnvelopeTests` asserts boundedness and repeatability.

### B) Extended training state (theta + EMA + step)
- State: { θ, EMA(θ), step_counter }
- Transition (derived): θ_next = θ + Δθ, EMA_next = β·EMA + (1−β)·θ_next, step_next = step + 1
- Admissibility constraints are enforced on the *next state* (θ_next, EMA_next, gap ||EMA_next−θ_next||, sign constraints, and exact step monotonicity).
- Result: baseline corrupts state; ASE Scale/Project keeps the entire derived next-state inside the envelope and fail-closes on invalid proposals.
- Test: `ASE_LearningStateEnvelopeTests` asserts boundedness, step monotonicity, and repeatability.

### C) Optimizer-state envelope (Adam-like training loop)
- State: { θ, m, v, EMA(θ), step_counter }  
  where `m, v` are first- and second-moment accumulators (Adam-style).
- Proposed step: Δθ (gradient-like signal with noise, rare spikes, rare NaN/Inf injections).
- Transition (derived, deterministic):
  - m_next = β₁·m + (1−β₁)·Δθ_eff  
  - v_next = β₂·v + (1−β₂)·(Δθ_eff ⊙ Δθ_eff)  
  - θ_next = θ − lr · m̂ / (√v̂ + ε)  
  - EMA_next = β·EMA + (1−β)·θ_next  
  - step_next = step + 1
- Admissibility constraints are enforced on the *entire next optimizer state*:
  - θ_next, m_next, v_next, EMA_next finite
  - ||θ_next||₂ ≤ R_θ
  - ||m_next||₂ ≤ R_m
  - ||v_next||₂ ≤ R_v and v_next ≥ 0
  - ||EMA_next||₂ ≤ R_EMA
  - ||EMA_next − θ_next||₂ ≤ R_gap
  - ||Δθ_eff||_∞ ≤ L (effective parameter displacement bound)
  - sign(θ_next[i]) ≥ 0 for i < K
  - step_next == step + 1 (exact monotonicity)
- Result: baseline without ASE corrupts parameters and optimizer buffers under spikes and non-finite proposals; ASE Scale/Project gates Δθ such that the entire derived next-state remains admissible and fail-closes to a neutral update when required.
- Test: `ASE_LearningAdamStateEnvelopeTests` asserts boundedness of all optimizer-state variables, non-negativity of v, exact step monotonicity, and deterministic repeatability.
