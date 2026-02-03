#pragma once
#include <cstddef>
#include <cstdint>

namespace ase {

// Enforcement modes (Specification §6)
enum class Mode : std::uint8_t {
    Reject  = 0,
    Scale   = 1,
    Project = 2
};

// Fixed configuration (Specification §6.1, Design §6)
struct Config final {
    Mode mode = Mode::Reject;

    // Scale-mode bounds (Specification §9 bounded execution)
    std::size_t max_scale_attempts = 16; // explicit bound
    double scale_factor = 0.5;           // monotonic toward 0
};

// Domain dependencies are injected (Design §5.4, Implementation Notes §4–6)
// IMPORTANT: API boundary returns ONLY Step (Integration Constraints §2.1)
template <class State, class Step>
struct Dependencies final {
    // Admissibility predicate: deterministic, stateless, local (Specification §7)
    bool (*is_admissible)(const State&, const Step&) = nullptr;

    // Neutral step provider: deterministic no-op (Specification §10)
    Step (*neutral_step)() = nullptr;

    // Step scaling: deterministic, side-effect free (used only in Scale mode)
    // Returns true if out is produced, false => treated fail-closed.
    bool (*scale_step)(const Step& in, double k, Step& out) = nullptr;

    // Step projection: deterministic, at most once (used only in Project mode)
    // Returns true if out is produced, false => treated fail-closed.
    bool (*project_step)(const State& S, const Step& in, Step& out) = nullptr;
};

// ASE core engine: stateless per call, bounded, deterministic (Specification §4, §9)
template <class State, class Step>
class Engine final {
public:
    Engine(const Config& cfg, const Dependencies<State, Step>& deps) noexcept
        : cfg_(cfg), deps_(deps) {}

    // Canonical enforcement entry point:
    // Input: (S, ΔS)  Output: ΔS' only (Integration Constraints §2.1)
    Step enforce(const State& S, const Step& proposed) const noexcept {
        // Fail-closed if critical hooks missing
        if (!deps_.is_admissible || !deps_.neutral_step) {
            return neutral_safe();
        }

        // 1) Evaluate admissibility of proposed step (Implementation Notes §3.3)
        bool admissible = false;
        if (!safe_is_admissible(S, proposed, admissible)) {
            // Evaluation failure => fail-closed => neutral (Specification §3.5, §11)
            return neutral_safe();
        }

        if (admissible) {
            // Pass-through (Specification §5.2)
            return proposed;
        }

        // 2) Inadmissible => enforce according to fixed mode (Specification §6.1)
        switch (cfg_.mode) {
            case Mode::Reject:
                // Reject => neutral (Specification §6.2)
                return neutral_safe();

            case Mode::Scale:
                // Scale => bounded attempts then neutral (Specification §6.3, §9)
                return enforce_scale(S, proposed);

            case Mode::Project:
                // Project once then neutral (Specification §6.4)
                return enforce_project(S, proposed);
        }

        // Defensive fail-closed (should not happen)
        return neutral_safe();
    }

private:
    Step enforce_scale(const State& S, const Step& proposed) const noexcept {
        if (!deps_.scale_step) return neutral_safe();

        double k = 1.0; // start from 1.0 (Specification §6.3)
        for (std::size_t i = 0; i < cfg_.max_scale_attempts; ++i) {
            Step scaled{};
            if (!deps_.scale_step(proposed, k, scaled)) {
                // Transform failure => neutral (fail-closed)
                return neutral_safe();
            }

            bool admissible = false;
            if (!safe_is_admissible(S, scaled, admissible)) {
                return neutral_safe();
            }

            if (admissible) {
                return scaled;
            }

            // monotonic toward 0 (Specification §6.3)
            k *= cfg_.scale_factor;
        }

        // Bounded attempts exhausted => neutral (Specification §5.4)
        return neutral_safe();
    }

    Step enforce_project(const State& S, const Step& proposed) const noexcept {
        if (!deps_.project_step) return neutral_safe();

        Step projected{};
        if (!deps_.project_step(S, proposed, projected)) {
            return neutral_safe();
        }

        bool admissible = false;
        if (!safe_is_admissible(S, projected, admissible)) {
            return neutral_safe();
        }

        if (!admissible) {
            // Project applied at most once; inadmissible => neutral (Specification §6.4)
            return neutral_safe();
        }

        return projected;
    }

    // MUST NOT allow exceptions to escape enforcement boundary (Specification §11.3)
    bool safe_is_admissible(const State& S, const Step& dS, bool& out) const noexcept {
#if defined(__cpp_exceptions)
        try {
            out = deps_.is_admissible(S, dS);
            return true;
        } catch (...) {
            out = false;
            return false;
        }
#else
        out = deps_.is_admissible(S, dS);
        return true;
#endif
    }

    Step neutral_safe() const noexcept {
        // Neutral step MUST be provided by integrator (Specification §10.3)
        if (deps_.neutral_step) return deps_.neutral_step();
        return Step{}; // last-resort fallback (still no I/O, fail-closed intent)
    }

private:
    Config cfg_;
    Dependencies<State, Step> deps_;
};

} // namespace ase
