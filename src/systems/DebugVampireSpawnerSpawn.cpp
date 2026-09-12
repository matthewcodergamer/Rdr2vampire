#include "nightwalker/systems/DebugVampireSpawner.h"

#include <array>
#include <cmath>
#include <string>

namespace nightwalker::systems {
namespace {
float DistSq(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x*x + y*y + z*z;
}
float NormalizeHeading(float h) noexcept {
    h = std::fmod(h, 360.0F);
    return h < 0.0F ? h + 360.0F : h;
}
}

bool DebugVampireSpawner::FindSpawnPoint(
    game::PedHandle player, game::Vec3& position, float& heading) const noexcept {
    if (!api_.PedAlive(player)) return false;
    const game::Vec3 playerPos = api_.EntityCoords(player);
    constexpr std::array<game::Vec3,5> offsets{{
        {0,4,0},{4,0,0},{-4,0,0},{0,-4,0},{0,6,0}
    }};
    for (const auto& o : offsets) {
        game::Vec3 candidate = api_.OffsetFromEntity(player, o.x, o.y, o.z), safe{};
        if (!api_.FindSafeCoordForPed(candidate, safe)) continue;
        const float d = DistSq(playerPos, safe);
        if (d < 9.0F || d > 144.0F || std::fabs(safe.z-playerPos.z) > 3.5F) continue;
        float water = 0.0F;
        if (api_.HasWaterAt(safe, water) && water >= safe.z - 0.25F) continue;
        position = safe;
        heading = NormalizeHeading(api_.EntityHeading(player)+180.0F);
        return true;
    }
    return false;
}

bool DebugVampireSpawner::SpawnLoadedModel(std::uint64_t nowMs) noexcept {
    if (registry_.Ped() != 0) {
        ResetRequest();
        return false;
    }
    const auto player = api_.PlayerPed();
    game::Vec3 point{};
    float heading = 0.0F;
    if (!FindSpawnPoint(player, point, heading)) {
        logger_.Write(util::LogLevel::Error, "No safe nearby spawn coordinate found for cs_vampire.");
        ResetRequest();
        return false;
    }
    auto ped = api_.CreateLocalPed(kVampireModel, point, heading);
    if (ped == 0 || !api_.EntityExists(ped) || !api_.PedAlive(ped) ||
        api_.EntityModel(ped) != kVampireModel) {
        if (ped != 0 && api_.EntityExists(ped) && api_.EntityModel(ped) == kVampireModel) {
            api_.DeletePed(ped);
        }
        ResetRequest();
        return false;
    }
    if (!registry_.Claim(ped, BossOwner::Debug)) {
        api_.DeletePed(ped);
        ResetRequest();
        return false;
    }
    ownedPed_ = ped;
    registry_.SetCombatEnabled(ownedPed_, BossOwner::Debug, true);
    state_ = State::Spawned;
    const auto elapsed = nowMs - modelRequest_.StartedAtMs();
    modelRequest_.Release(api_);
    logger_.Write(util::LogLevel::Info,
        std::string("Spawned debug cs_vampire handle=") + std::to_string(ownedPed_) +
        " modelLoadMs=" + std::to_string(elapsed));
    return true;
}

} // namespace nightwalker::systems
