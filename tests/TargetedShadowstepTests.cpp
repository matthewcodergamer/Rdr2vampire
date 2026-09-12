#include "nightwalker/core/Config.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/systems/ShadowstepMath.h"
#include "nightwalker/systems/ShadowstepResolver.h"
#include "nightwalker/systems/TargetedShadowstepPlanner.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void Check(bool value, const char* name) {
    if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}

class FakeApi final : public nightwalker::game::IGameApi {
public:
    bool safe{true};
    float groundZ{10.0F};

    nightwalker::game::PedHandle PlayerPed() const noexcept override { return 1; }
    bool EntityExists(nightwalker::game::EntityHandle entity) const noexcept override { return entity != 0; }
    bool PedAlive(nightwalker::game::PedHandle ped) const noexcept override { return ped != 0; }
    nightwalker::game::ModelHash EntityModel(nightwalker::game::EntityHandle) const noexcept override { return 1; }
    nightwalker::game::Vec3 EntityCoords(nightwalker::game::EntityHandle) const noexcept override { return {}; }
    float EntityHeading(nightwalker::game::EntityHandle) const noexcept override { return 0.0F; }
    nightwalker::game::Vec3 EntityForward(nightwalker::game::EntityHandle) const noexcept override { return {0,1,0}; }
    nightwalker::game::Vec3 OffsetFromEntity(nightwalker::game::EntityHandle, float x, float y, float z) const noexcept override { return {x,y,z}; }
    bool FindSafeCoordForPed(const nightwalker::game::Vec3& nearPosition, nightwalker::game::Vec3& out) const noexcept override { if (!safe) return false; out = nearPosition; return true; }
    bool TryGroundZ(const nightwalker::game::Vec3&, float, float& out) const noexcept override { if (!safe) return false; out = groundZ; return true; }
    bool HasWaterAt(const nightwalker::game::Vec3&, float& out) const noexcept override { out = 0; return false; }
    nightwalker::game::RaycastResult RaycastWorld(const nightwalker::game::Vec3&, const nightwalker::game::Vec3& end, nightwalker::game::EntityHandle) const noexcept override { nightwalker::game::RaycastResult r{}; r.conclusive = safe; r.endCoords = end; return r; }
    bool SetEntityCoordsNoOffset(nightwalker::game::EntityHandle, const nightwalker::game::Vec3&) noexcept override { return true; }
    bool IsPedModelAvailable(nightwalker::game::ModelHash) const noexcept override { return false; }
    void RequestModel(nightwalker::game::ModelHash) noexcept override {}
    bool IsModelLoaded(nightwalker::game::ModelHash) const noexcept override { return false; }
    void ReleaseModel(nightwalker::game::ModelHash) noexcept override {}
    nightwalker::game::PedHandle CreateLocalPed(nightwalker::game::ModelHash, const nightwalker::game::Vec3&, float) noexcept override { return 0; }
    bool DeletePed(nightwalker::game::PedHandle&) noexcept override { return true; }
};
}

int main() {
    using namespace nightwalker;
    core::ShadowstepSettings settings{};
    settings.maxVerticalDelta = 1.5;
    settings.wallClearance = 0.65;

    FakeApi api;
    systems::ShadowstepResolver resolver(api, settings);
    systems::TargetedShadowstepPlanner planner(resolver);

    systems::TargetedShadowstepRequest request{};
    request.actor = 10;
    request.target = 1;
    request.actorPosition = {0.0F, -6.0F, 10.0F};
    request.targetPosition = {0.0F, 0.0F, 10.0F};
    request.targetForward = {0.0F, -1.0F, 0.0F};
    request.targetVelocity = {0.0F, 2.0F, 0.0F};
    request.strikingRange = 2.9F;
    request.predictionSeconds = 0.25F;
    request.maxPredictionMeters = 1.75F;
    request.preferIntercept = true;

    auto intercept = planner.Plan(request);
    Check(intercept.valid, "retreating target produces plan");
    Check(intercept.chosenType == systems::ShadowstepCandidateType::Intercept,
          "retreat preference chooses intercept");
    Check(intercept.destination.y > request.targetPosition.y,
          "intercept leads target movement");

    request.preferIntercept = false;
    request.preferEvade = true;
    auto evade = planner.Plan(request);
    Check(evade.valid, "evade produces plan");
    Check(evade.chosenType == systems::ShadowstepCandidateType::LeftFlank ||
          evade.chosenType == systems::ShadowstepCandidateType::RightFlank,
          "evade prefers a lateral candidate");

    api.safe = false;
    auto unsafe = planner.Plan(request);
    Check(!unsafe.valid, "unsafe geometry falls back with no plan");
    for (const auto& candidate : unsafe.candidates) {
        Check(!candidate.resolution.valid, "unsafe candidate rejected");
    }

    std::string diagnostics;
    auto config = core::Config::Parse(
        "[VampireAI]\n"
        "ShadowstepMinDistance=0\n"
        "ShadowstepMaxDistance=1\n"
        "StrikingRange=9\n"
        "PredictionMs=9999\n"
        "ShadowstepCooldownMs=1\n"
        "TelegraphMs=1\n"
        "EvadeCooldownMs=1\n",
        [&](std::string_view message) { diagnostics += message; diagnostics += '\n'; });
    Check(config.vampireAi.shadowstepMinDistance == 2.5, "AI minimum range clamped");
    Check(config.vampireAi.shadowstepMaxDistance >= 5.0, "AI maximum range clamped");
    Check(config.vampireAi.strikingRange == 2.5, "AI striking range clamped");
    Check(config.vampireAi.predictionMs == 600, "AI prediction clamped");
    Check(config.vampireAi.shadowstepCooldownMs == 1200, "AI cooldown clamped");
    Check(config.vampireAi.telegraphMs == 180, "AI telegraph clamped");
    Check(config.vampireAi.evadeCooldownMs == 2500, "AI evade cooldown clamped");
    Check(!diagnostics.empty(), "AI config diagnostics emitted");

    if (failures) {
        std::cerr << failures << " targeted Shadowstep test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Nightwalker targeted Shadowstep tests passed\n";
    return EXIT_SUCCESS;
}
