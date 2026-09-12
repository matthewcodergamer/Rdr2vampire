#include "nightwalker/core/Config.h"
#include "nightwalker/systems/MovementMath.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void Check(bool value, const char* name) { if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; } }
bool Near(float a, float b, float epsilon = 0.001F) { return std::fabs(a - b) <= epsilon; }
}

int main() {
    using namespace nightwalker;
    const auto defaults = core::Config::Parse("");
    Check(defaults.movement.enabled, "movement enabled by default");
    Check(defaults.movement.sprintMoveRate == 1.15, "default move rate");
    Check(defaults.movement.accelerationMs == 350, "default acceleration");
    Check(defaults.movement.burstDurationMs == 1800, "default burst duration");
    Check(defaults.movement.recoveryMs == 900, "default recovery");

    std::string diagnostics;
    const auto clamped = core::Config::Parse(
        "[Movement]\nSprintMoveRate=9\nAccelerationMs=1\nBurstDurationMs=99999\nRecoveryMs=1\nDismountRecoveryMs=1\nActivationDistance=99\nMinVelocity=-2\nTrailIntervalMs=1\n",
        [&](std::string_view message) { diagnostics += message; diagnostics += '\n'; });
    Check(clamped.movement.sprintMoveRate == 1.20, "move rate maximum");
    Check(clamped.movement.accelerationMs == 100, "acceleration minimum");
    Check(clamped.movement.burstDurationMs == 4000, "burst duration maximum");
    Check(clamped.movement.recoveryMs == 250, "recovery minimum");
    Check(clamped.movement.dismountRecoveryMs == 150, "dismount recovery minimum");
    Check(clamped.movement.activationDistance == 15.0, "activation distance maximum");
    Check(clamped.movement.minVelocity == 0.05, "minimum velocity bound");
    Check(clamped.movement.trailIntervalMs == 100, "trail interval minimum");
    Check(!diagnostics.empty(), "movement diagnostics emitted");

    const auto alias = core::Config::Parse("[Movement]\nMaxBurstMs=1200\nTrailFx=false\n");
    Check(alias.movement.burstDurationMs == 1200, "MaxBurstMs alias parses");
    Check(!alias.movement.trailFx, "trail feature flag parses");

    using systems::movement_math::HorizontalSpeed;
    using systems::movement_math::RampMultiplier;
    using systems::movement_math::SmoothStep01;
    Check(Near(SmoothStep01(-1.0F), 0.0F), "smooth step low bound");
    Check(Near(SmoothStep01(2.0F), 1.0F), "smooth step high bound");
    Check(Near(RampMultiplier(0, 400, 1.15F), 1.0F), "ramp starts at normal speed");
    const float halfway = RampMultiplier(200, 400, 1.15F);
    Check(halfway > 1.0F && halfway < 1.15F, "ramp midpoint is gradual");
    Check(Near(RampMultiplier(400, 400, 1.15F), 1.15F), "ramp reaches target");
    Check(Near(RampMultiplier(900, 400, 1.15F), 1.15F), "ramp stays at target");
    Check(Near(HorizontalSpeed({3.0F, 4.0F, 12.0F}), 5.0F), "horizontal speed ignores vertical component");

    if (failures != 0) { std::cerr << failures << " movement test(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Nightwalker supernatural movement tests passed\n";
    return EXIT_SUCCESS;
}
