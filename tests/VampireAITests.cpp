#include "nightwalker/core/Config.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/systems/ShadowstepResolver.h"
#include "nightwalker/systems/TargetedShadowstepPlanner.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void Check(bool value, const char* name) {
    if (!value) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

class FakeApi final : public nightwalker::game::IGameApi {
public:
    bool blocked{false};
    bool unsafeWater{false};

    nightwalker::game::PedHandle PlayerPed() const noexcept override { return 1; }
    bool EntityExists(nightwalker::game::EntityHandle entity) const noexcept override { return entity != 0; }
    bool PedAlive(nightwalker::game::PedHandle ped) const noexcept override { return ped != 0; }
    nightwalker::game::ModelHash EntityModel(nightwalker::game::EntityHandle) const noexcept override { return 0; }
    nightwalker::game::Vec3 EntityCoords(nightwalker::game::EntityHandle) const noexcept override { return {}; }
    float EntityHeading(nightwalker::game::EntityHandle) const noexcept override { return 0.0F; }
    nightwalker::game::Vec3 OffsetFromEntity(nightwalker::game::EntityHandle, float x, float y, float z) const noexcept override { return {x, y, z}; }
    bool FindSafeCoordForPed(const nightwalker::game::Vec3& nearPosition, nightwalker::game::Vec3& safePosition) const noexcept override {
        safePosition = nearPosition;
        return true;
    }
    bool TryGroundZ(const nightwalker::game::Vec3&, float, float& groundZ) const noexcept override {
        groundZ = 0.0F;
        return true;
    }
    bool HasWaterAt(const nightwalker::game::Vec3&, float& waterHeight) const noexcept override {
        waterHeight = unsafeWater ? 2.0F : 0.0F;
        return unsafeWater;
    }
    nightwalker::game::RaycastResult RaycastWorld(
        const nightwalker::game::Vec3& start,
        const nightwalker::game::Vec3& end,
        nightwalker::game::EntityHandle) const noexcept override {
        nightwalker::game::RaycastResult result{};
        result.conclusive = true;
        result.hit = blocked;
        result.endCoords = blocked
            ? nightwalker::game::Vec3{(start.x + end.x) * 0.5F, (start.y + end.y) * 0.5F, (start.z + end.z) * 0.5F}
            : end;
        return result;
    }
    bool IsPedModelAvailable(nightwalker::game::ModelHash) const noexcept override { return false; }
    void RequestModel(nightwalker::game::ModelHash) noexcept override {}
    bool IsModelLoaded(nightwalker::game::ModelHash) const noexcept override { return false; }
    void ReleaseModel(nightwalker::game::ModelHash) noexcept override {}
    nightwalker::game::PedHandle CreateLocalPed(nightwalker::game::ModelHash, const nightwalker::game::Vec3&, float) noexcept override { return 0; }
    bool DeletePed(nightwalker::game::PedHandle&) noexcept override { return false; }
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
    request.actor = 2;
    request.target = 1;
    request.actorPosition = {0.0F, -7.0F, 0.05F};
    request.targetPosition = {0.0F, 0.0F, 0.05F};
    request.targetForward = {0.0F, 1.0F, 0.0F};
    request.targetVelocity = {0.0F, 2.0F, 0.0F};
    request.strikingRange = 1.65F;
    request.predictionSeconds = 0.25F;
    request.maxPredictionMeters = 1.75F;
    request.preferIntercept = true;

    const auto intercept = planner.Plan(request);
    Check(intercept.valid, "moving target produces a valid plan");
    Check(intercept.chosenType == systems::ShadowstepCandidateType::Intercept,
          "retreat preference chooses intercept candidate");
    Check(std::hypot(intercept.destination.x, intercept.destination.y) > 1.0F,
          "intercept does not land inside target capsule");

    request.preferIntercept = false;
    request.preferEvade = true;
    request.targetVelocity = {};
    const auto evade = planner.Plan(request);
    Check(evade.valid, "evade request produces a valid plan");
    Check(evade.chosenType == systems::ShadowstepCandidateType::LeftFlank ||
          evade.chosenType == systems::ShadowstepCandidateType::RightFlank,
          "evade preference chooses a lateral flank");

    api.blocked = true;
    const auto blocked = planner.Plan(request);
    Check(!blocked.valid, "fully blocked geometry safely rejects every candidate");

    api.blocked = false;
    api.unsafeWater = true;
    const auto water = planner.Plan(request);
    Check(!water.valid, "unsafe water rejects every candidate");

    std::string diagnostics;
    const auto config = core::Config::Parse(
        "[VampireAI]\nEnabled=true\nShadowstepMinDistance=-5\nShadowstepMaxDistance=99\n"
        "ShadowstepCooldownMs=1\nTelegraphMs=1\nEvadeCooldownMs=1\nPredictionMs=9999\n",
        [&](std::string_view message) { diagnostics += message; diagnostics += '\n'; });
    Check(config.vampireAi.enabled, "VampireAI feature flag parses");
    Check(config.vampireAi.shadowstepMinDistance >= 2.0, "minimum teleport range clamps safely");
    Check(config.vampireAi.shadowstepMaxDistance <= 15.0, "maximum teleport range clamps safely");
    Check(config.vampireAi.shadowstepCooldownMs >= 1200, "anti-spam cooldown clamps safely");
    Check(config.vampireAi.telegraphMs >= 150, "readable telegraph clamps safely");
    Check(config.vampireAi.evadeCooldownMs >= 2500, "evade rate limit clamps safely");
    Check(config.vampireAi.predictionMs <= 600, "prediction remains conservative");
    Check(!diagnostics.empty(), "unsafe AI values emit diagnostics");

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Nightwalker Vampire AI planner tests passed\n";
    return EXIT_SUCCESS;
}
