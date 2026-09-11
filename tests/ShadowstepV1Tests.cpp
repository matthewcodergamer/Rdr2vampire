#include "nightwalker/core/Config.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/systems/ShadowstepMath.h"
#include "nightwalker/systems/ShadowstepResolver.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
int failures = 0;
void Check(bool value, const char* name) { if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; } }
bool Near(float a, float b, float epsilon = 0.03F) { return std::fabs(a - b) <= epsilon; }

class FakeApi final : public nightwalker::game::IGameApi {
public:
    nightwalker::game::Vec3 coords{0.0F, 0.0F, 10.0F};
    nightwalker::game::Vec3 forward{0.0F, 1.0F, 0.0F};
    nightwalker::game::Vec3 safePoint{};
    bool explicitSafePoint{false};
    bool safePointAvailable{true};
    bool groundAvailable{true};
    float groundZ{10.0F};
    bool waterPresent{false};
    float waterHeight{0.0F};
    mutable std::size_t traceIndex{0};
    std::vector<nightwalker::game::RaycastResult> traces{};

    nightwalker::game::PedHandle PlayerPed() const noexcept override { return 1; }
    bool EntityExists(nightwalker::game::EntityHandle) const noexcept override { return true; }
    bool PedAlive(nightwalker::game::PedHandle) const noexcept override { return true; }
    nightwalker::game::ModelHash EntityModel(nightwalker::game::EntityHandle) const noexcept override { return 0; }
    nightwalker::game::Vec3 EntityCoords(nightwalker::game::EntityHandle) const noexcept override { return coords; }
    float EntityHeading(nightwalker::game::EntityHandle) const noexcept override { return 0.0F; }
    nightwalker::game::Vec3 EntityForward(nightwalker::game::EntityHandle) const noexcept override { return forward; }
    nightwalker::game::Vec3 OffsetFromEntity(nightwalker::game::EntityHandle, float x, float y, float z) const noexcept override { return {coords.x + x, coords.y + y, coords.z + z}; }
    bool FindSafeCoordForPed(const nightwalker::game::Vec3& nearPosition, nightwalker::game::Vec3& out) const noexcept override { if (!safePointAvailable) return false; out = explicitSafePoint ? safePoint : nearPosition; return true; }
    bool TryGroundZ(const nightwalker::game::Vec3&, float, float& out) const noexcept override { if (!groundAvailable) return false; out = groundZ; return true; }
    bool HasWaterAt(const nightwalker::game::Vec3&, float& out) const noexcept override { out = waterHeight; return waterPresent; }
    nightwalker::game::RaycastResult RaycastWorld(const nightwalker::game::Vec3&, const nightwalker::game::Vec3& end, nightwalker::game::EntityHandle) const noexcept override { if (traceIndex < traces.size()) return traces[traceIndex++]; nightwalker::game::RaycastResult r{}; r.conclusive = true; r.endCoords = end; return r; }
    bool SetEntityCoordsNoOffset(nightwalker::game::EntityHandle, const nightwalker::game::Vec3& position) noexcept override { coords = position; return true; }
    bool IsPedModelAvailable(nightwalker::game::ModelHash) const noexcept override { return false; }
    void RequestModel(nightwalker::game::ModelHash) noexcept override {}
    bool IsModelLoaded(nightwalker::game::ModelHash) const noexcept override { return false; }
    void ReleaseModel(nightwalker::game::ModelHash) noexcept override {}
    nightwalker::game::PedHandle CreateLocalPed(nightwalker::game::ModelHash, const nightwalker::game::Vec3&, float) noexcept override { return 0; }
    bool DeletePed(nightwalker::game::PedHandle&) noexcept override { return true; }
    void ResetTraces() { traces.clear(); traceIndex = 0; }
};
}

int main() {
    using namespace nightwalker;
    using namespace systems::shadowstep_math;

    game::Vec3 normalized{};
    Check(NormalizeHorizontal({3.0F, 4.0F, 9.0F}, normalized), "normalize horizontal");
    Check(Near(normalized.x, 0.6F) && Near(normalized.y, 0.8F) && Near(normalized.z, 0.0F), "normalize value");
    Check(!NormalizeHorizontal({0.0F, 0.0F, 1.0F}, normalized), "reject vertical-only direction");
    Check(Near(AddScaled({0,0,10}, {0,1,0}, 6.5F).y, 6.5F), "range projection");
    Check(VerticalDeltaWithin({0,0,10}, {0,0,11.5F}, 1.5F), "vertical limit inclusive");
    Check(!VerticalDeltaWithin({0,0,10}, {0,0,11.6F}, 1.5F), "vertical limit rejects");

    std::string diagnostics;
    auto parsed = core::Config::Parse("[Debug]\nShadowstepHotkey=0x77\n[Shadowstep]\nMaxVerticalRise=99\nValidationTimeoutMs=1\nWallClearance=9\n", [&](std::string_view m){ diagnostics += m; diagnostics += '\n'; });
    Check(parsed.debug.shadowstepHotkey == 0x76, "reserved debug key resets to F7");
    Check(parsed.shadowstep.maxVerticalDelta == 3.0, "vertical delta clamped");
    Check(parsed.shadowstep.validationTimeoutMs == 50, "validation timeout clamped");
    Check(parsed.shadowstep.wallClearance == 1.5, "wall clearance clamped");
    Check(!diagnostics.empty(), "config diagnostics emitted");

    core::ShadowstepSettings settings{};
    settings.quickDistance = 6.5;
    settings.maxVerticalDelta = 1.5;
    settings.wallClearance = 0.65;
    FakeApi api;
    systems::ShadowstepResolver resolver(api, settings);
    const game::Vec3 start{0,0,10};
    const game::Vec3 direction{0,1,0};

    auto full = resolver.Resolve(1, start, direction);
    Check(full.valid && !full.shortened, "full-range destination valid");
    Check(Near(full.finalPosition.y, 6.5F), "full-range endpoint");

    api.ResetTraces();
    game::RaycastResult wall{}; wall.conclusive = true; wall.hit = true; wall.endCoords = {0,4.0F,10.85F}; api.traces.push_back(wall);
    auto shortened = resolver.Resolve(1, start, direction);
    Check(shortened.valid && shortened.shortened, "wall shortens destination");
    Check(shortened.finalDistance > 3.0F && shortened.finalDistance < 3.6F, "wall clearance preserved");

    api.ResetTraces(); api.groundZ = 14.0F;
    auto vertical = resolver.Resolve(1, start, direction);
    Check(!vertical.valid && vertical.reason == systems::ShadowstepRejectReason::VerticalDeltaTooLarge, "unsafe vertical delta rejected");

    api.groundZ = 10.0F; api.waterPresent = true; api.waterHeight = 11.0F; api.ResetTraces();
    auto water = resolver.Resolve(1, start, direction);
    Check(!water.valid && water.reason == systems::ShadowstepRejectReason::DeepWater, "deep water rejected");

    api.waterPresent = false; api.ResetTraces(); api.traces.push_back({});
    auto unknown = resolver.Resolve(1, start, direction);
    Check(!unknown.valid && unknown.reason == systems::ShadowstepRejectReason::TraceInconclusive, "inconclusive trace rejected");

    api.ResetTraces(); game::RaycastResult clear{}; clear.conclusive = true; api.traces.push_back(clear); game::RaycastResult blocked{}; blocked.conclusive = true; blocked.hit = true; api.traces.push_back(blocked);
    auto clearance = resolver.Resolve(1, start, direction);
    Check(!clearance.valid && clearance.reason == systems::ShadowstepRejectReason::ClearanceBlocked, "destination clearance rejected");

    if (failures) { std::cerr << failures << " test(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Nightwalker Shadowstep V1 tests passed\n";
    return EXIT_SUCCESS;
}
