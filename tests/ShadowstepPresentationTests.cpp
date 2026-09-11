#include "nightwalker/systems/ShadowstepPresentationSettings.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void Check(bool value, const char* name) {
    if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
bool Near(float a, float b, float epsilon = 0.001F) { return std::fabs(a - b) <= epsilon; }
}

int main() {
    namespace fs = std::filesystem;
    using nightwalker::systems::LoadShadowstepPresentationSettings;

    const fs::path temp = fs::temp_directory_path() / "nightwalker_phase4_test.ini";
    {
        std::ofstream stream(temp, std::ios::trunc);
        stream << "[Shadowstep]\n"
               << "DisappearMs=999\n"
               << "ArrivalCarryMeters=9\n"
               << "ArrivalCarryMs=1\n"
               << "MeleeBufferMs=9999\n"
               << "StateTimeoutMs=1\n"
               << "SmokeFx=false\n";
    }

    std::string diagnostics;
    const auto clamped = LoadShadowstepPresentationSettings(
        temp, [&](std::string_view message) { diagnostics += message; diagnostics += '\n'; });
    Check(clamped.disappearMs == 130, "disappear clamp");
    Check(Near(clamped.carryMeters, 2.0F), "carry distance clamp");
    Check(clamped.carryMs == 80, "carry duration clamp");
    Check(clamped.meleeBufferMs == 500, "melee buffer clamp");
    Check(clamped.stateTimeoutMs == 300, "state timeout clamp");
    Check(!clamped.smokeFx, "smoke toggle parsed");
    Check(!diagnostics.empty(), "clamp diagnostics emitted");

    {
        std::ofstream stream(temp, std::ios::trunc);
        stream << "[Shadowstep]\n"
               << "DisappearMs=bad\n"
               << "ArrivalCarryMeters=bad\n"
               << "AttackBufferMs=180\n";
    }
    diagnostics.clear();
    const auto malformed = LoadShadowstepPresentationSettings(
        temp, [&](std::string_view message) { diagnostics += message; diagnostics += '\n'; });
    Check(malformed.disappearMs == 110, "malformed disappear keeps default");
    Check(Near(malformed.carryMeters, 1.25F), "malformed carry keeps default");
    Check(malformed.meleeBufferMs == 180, "legacy attack buffer alias accepted");
    Check(!diagnostics.empty(), "malformed diagnostics emitted");

    std::error_code ignored;
    fs::remove(temp, ignored);

    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Nightwalker Shadowstep presentation tests passed\n";
    return EXIT_SUCCESS;
}
