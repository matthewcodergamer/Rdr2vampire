#include "nightwalker/systems/SaintDenisDirector.h"

#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {
namespace {
constexpr std::array<game::Vec3, 8> kSpawnOffsets{{
    {20.0F, 0.0F, 0.0F}, {-20.0F, 0.0F, 0.0F},
    {0.0F, 20.0F, 0.0F}, {0.0F, -20.0F, 0.0F},
    {28.0F, 16.0F, 0.0F}, {-28.0F, 16.0F, 0.0F},
    {28.0F, -16.0F, 0.0F}, {-28.0F, -16.0F, 0.0F},
}};

float NormalizeHeading(float heading) noexcept {
    heading = std::fmod(heading, 360.0F);
    return heading < 0.0F ? heading + 360.0F : heading;
}
}

SaintDenisDirector::SaintDenisDirector(
    game::IGameApi& api, game::IGameCombatApi& combatApi,
    game::IGameEncounterApi& encounterApi, game::IGamePresentationApi& presentationApi,
    BossActorRegistry& registry, narrative::NarrativeController& narrative,
    narrative::ReactiveConversationController& conversation,
    ui::BossHudController& bossHud, util::Logger& logger, const core::Config& config) noexcept
    : api_(api), combatApi_(combatApi), encounterApi_(encounterApi),
      presentationApi_(presentationApi), registry_(registry), narrative_(narrative),
      conversation_(conversation), bossHud_(bossHud), logger_(logger), config_(config) {}

bool SaintDenisDirector::Initialize() {
    modelRequest_.Release(api_);
    bossHud_.ForceHide();
    state_ = SaintDenisState::Dormant;
    actor_ = 0;
    stateStartedMs_ = 0;
    nextEligibilityCheckMs_ = 0;
    nextOmenMs_ = 0;
    nextSpawnRetryMs_ = 0;
    outsideSinceMs_ = 0;
    cooldownUntilGameSeconds_ = 0;
    pendingConversationIntent_ = narrative::ConversationIntent::None;
    cleanupResolved_ = false;
    resolvedThisSession_ = false;
    logger_.Write(util::LogLevel::Info,
        "EncounterDirector initialized for the Saint Denis nighttime encounter.");
    return true;
}

bool SaintDenisDirector::EligibleNow() const noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::Encounter) ||
        !config_.IsFeatureEnabled(core::Feature::VampireAi) || registry_.Ped() != 0) return false;
    const auto player = api_.PlayerPed();
    if (!api_.PedAlive(player)) return false;
    if (!encounter_math::IsHourInWindow(
            encounterApi_.ClockHour(), config_.encounter.startHour, config_.encounter.endHour)) return false;
    if (!encounter_math::CooldownExpired(
            encounterApi_.GameSecondsSinceBaseYear(), cooldownUntilGameSeconds_)) return false;
    const game::Vec3 center{static_cast<float>(config_.encounter.centerX),
        static_cast<float>(config_.encounter.centerY), static_cast<float>(config_.encounter.centerZ)};
    return encounter_math::WithinRadius(
        api_.EntityCoords(player), center, static_cast<float>(config_.encounter.triggerRadius));
}

bool SaintDenisDirector::PreCombatStillSafe() const noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::Encounter) ||
        !config_.IsFeatureEnabled(core::Feature::VampireAi)) return false;
    const auto player = api_.PlayerPed();
    if (!api_.PedAlive(player)) return false;
    if (!encounter_math::IsHourInWindow(
            encounterApi_.ClockHour(), config_.encounter.startHour, config_.encounter.endHour)) return false;
    const game::Vec3 center{static_cast<float>(config_.encounter.centerX),
        static_cast<float>(config_.encounter.centerY), static_cast<float>(config_.encounter.centerZ)};
    if (!encounter_math::WithinRadius(
            api_.EntityCoords(player), center, static_cast<float>(config_.encounter.abortRadius))) return false;
    if (actor_ != 0) return registry_.Ped() == actor_ && registry_.Owner() == BossOwner::Encounter;
    return registry_.Ped() == 0;
}

bool SaintDenisDirector::ActorValid(bool requireAlive) const noexcept {
    if (actor_ == 0 || registry_.Ped() != actor_ || registry_.Owner() != BossOwner::Encounter) return false;
    if (!api_.EntityExists(actor_) || api_.EntityModel(actor_) != kSaintDenisVampireModel) return false;
    return !requireAlive || api_.PedAlive(actor_);
}

bool SaintDenisDirector::FindSpawnPoint(game::Vec3& position, float& heading, bool& visible) const noexcept {
    const auto player = api_.PlayerPed();
    if (!api_.PedAlive(player)) return false;
    const auto playerPos = api_.EntityCoords(player);
    const game::Vec3 center{static_cast<float>(config_.encounter.centerX),
        static_cast<float>(config_.encounter.centerY), static_cast<float>(config_.encounter.centerZ)};
    bool found = false;
    bool bestVisible = true;
    float bestScore = std::numeric_limits<float>::max();
    game::Vec3 best{};

    for (const auto& offset : kSpawnOffsets) {
        game::Vec3 candidate{center.x + offset.x, center.y + offset.y, center.z};
        game::Vec3 safe{};
        if (!api_.FindSafeCoordForPed(candidate, safe)) continue;
        float groundZ = 0.0F;
        if (!api_.TryGroundZ(safe, 6.0F, groundZ)) continue;
        safe.z = groundZ + 0.05F;
        if (std::fabs(safe.z - center.z) > 12.0F) continue;
        float waterHeight = 0.0F;
        if (api_.HasWaterAt(safe, waterHeight) && waterHeight >= safe.z - 0.25F) continue;
        const float distance = encounter_math::Distance2D(playerPos, safe);
        if (distance < static_cast<float>(config_.encounter.spawnMinDistance) ||
            distance > static_cast<float>(config_.encounter.spawnMaxDistance)) continue;
        const bool onScreen = encounterApi_.IsSphereVisible(safe, 1.25F);
        const float desired = static_cast<float>(
            (config_.encounter.spawnMinDistance + config_.encounter.spawnMaxDistance) * 0.5);
        const float score = std::fabs(distance - desired) + (onScreen ? 1000.0F : 0.0F);
        if (!found || score < bestScore) {
            found = true;
            bestScore = score;
            bestVisible = onScreen;
            best = safe;
        }
    }
    if (!found) return false;
    position = best;
    visible = bestVisible;
    heading = NormalizeHeading(api_.EntityHeading(player) + 180.0F);
    return true;
}

bool SaintDenisDirector::BeginModelRequest(std::uint64_t nowMs) noexcept {
    if (registry_.Ped() != 0) return false;
    const auto status = modelRequest_.Begin(api_, kSaintDenisVampireModel, nowMs,
        static_cast<std::uint64_t>(config_.encounter.spawnTimeoutMs));
    if (status == game::ModelStreamStatus::InvalidModel) {
        logger_.Write(util::LogLevel::Error, "Saint Denis encounter could not request cs_vampire.");
        return false;
    }
    return true;
}

bool SaintDenisDirector::SpawnActor(std::uint64_t nowMs) noexcept {
    game::Vec3 spawn{};
    float heading = 0.0F;
    bool visible = true;
    if (!FindSpawnPoint(spawn, heading, visible)) return false;
    auto ped = api_.CreateLocalPed(kSaintDenisVampireModel, spawn, heading);
    if (ped == 0 || !api_.EntityExists(ped) || !api_.PedAlive(ped) ||
        api_.EntityModel(ped) != kSaintDenisVampireModel) {
        if (ped != 0 && api_.EntityExists(ped) && api_.EntityModel(ped) == kSaintDenisVampireModel) api_.DeletePed(ped);
        logger_.Write(util::LogLevel::Error, "Saint Denis encounter failed to create a valid cs_vampire.");
        return false;
    }
    if (!registry_.Claim(ped, BossOwner::Encounter)) {
        api_.DeletePed(ped);
        logger_.Write(util::LogLevel::Warning, "Encounter spawn lost boss ownership; created ped removed.");
        return false;
    }
    actor_ = ped;
    registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
    combatApi_.TaskStandStill(actor_, 1200);
    modelRequest_.Release(api_);
    presentationApi_.PlayShadowSmoke(api_.EntityCoords(actor_), 0.55F);
    if (visible) logger_.Write(util::LogLevel::Warning,
        "Encounter spawn used a camera-visible fallback; no safe hidden candidate was available.");
    logger_.Write(util::LogLevel::Info,
        std::string("Saint Denis encounter spawned cs_vampire handle=") + std::to_string(actor_));
    Transition(SaintDenisState::Stalking, nowMs);
    return true;
}

} // namespace nightwalker::systems
