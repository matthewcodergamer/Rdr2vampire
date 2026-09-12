#include "nightwalker/systems/SaintDenisDirector.h"

#include <string>

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {

void SaintDenisDirector::BeginAbort(std::string_view reason, std::uint64_t nowMs) noexcept {
    if (state_ == SaintDenisState::Abort || state_ == SaintDenisState::Cleanup) return;
    if (actor_ != 0 && registry_.Ped() == actor_ && registry_.Owner() == BossOwner::Encounter) {
        registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
    }
    logger_.Write(util::LogLevel::Warning,
        std::string("Saint Denis encounter abort: ") + std::string(reason));
    cleanupResolved_ = false;
    Transition(SaintDenisState::Abort, nowMs);
}

bool SaintDenisDirector::CleanupActor(bool resolved) noexcept {
    modelRequest_.Release(api_);
    if (actor_ == 0) return true;

    const auto owned = actor_;
    if (registry_.Ped() == owned && registry_.Owner() == BossOwner::Encounter) {
        registry_.SetCombatEnabled(owned, BossOwner::Encounter, false);
    }

    if (!api_.EntityExists(owned)) {
        registry_.Release(owned, BossOwner::Encounter);
        actor_ = 0;
        return true;
    }
    if (api_.EntityModel(owned) != kSaintDenisVampireModel) {
        logger_.Write(util::LogLevel::Error,
            "Encounter actor handle was reused by another model; dropping ownership without deletion.");
        registry_.Release(owned, BossOwner::Encounter);
        actor_ = 0;
        return true;
    }

    combatApi_.ClearPedTasks(owned);
    presentationApi_.RestorePedAppearance(owned);
    game::PedHandle deleteHandle = owned;
    if (!api_.DeletePed(deleteHandle)) {
        logger_.Write(util::LogLevel::Error,
            "Encounter cleanup could not delete the owned cs_vampire; cleanup will retry.");
        return false;
    }

    registry_.Release(owned, BossOwner::Encounter);
    actor_ = 0;
    logger_.Write(util::LogLevel::Info,
        resolved ? "Saint Denis encounter actor cleaned after resolution." :
                   "Saint Denis encounter actor cleaned after abort.");
    return true;
}

void SaintDenisDirector::Cancel() noexcept {
    modelRequest_.Release(api_);
    cleanupResolved_ = false;
    if (actor_ != 0 && registry_.Ped() == actor_ && registry_.Owner() == BossOwner::Encounter) {
        registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
    }
    if (CleanupActor(false)) {
        cooldownUntilGameSeconds_ = encounter_math::AddCooldownMinutes(
            encounterApi_.GameSecondsSinceBaseYear(), config_.encounter.abortCooldownMinutes);
        state_ = SaintDenisState::Cooldown;
    } else {
        state_ = SaintDenisState::Cleanup;
    }
    stateStartedMs_ = 0;
    outsideSinceMs_ = 0;
}

void SaintDenisDirector::Shutdown() noexcept {
    Cancel();
    modelRequest_.Release(api_);
}

void SaintDenisDirector::Transition(SaintDenisState next, std::uint64_t nowMs) noexcept {
    if (config_.debug.enabled && state_ != next) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Encounter state ") + StateName(state_) + " -> " + StateName(next));
    }
    state_ = next;
    stateStartedMs_ = nowMs;
}

const char* SaintDenisDirector::StateName(SaintDenisState state) noexcept {
    switch (state) {
        case SaintDenisState::Dormant: return "Dormant";
        case SaintDenisState::Eligible: return "Eligible";
        case SaintDenisState::Omen: return "Omen";
        case SaintDenisState::SpawnPending: return "SpawnPending";
        case SaintDenisState::Stalking: return "Stalking";
        case SaintDenisState::Confrontation: return "Confrontation";
        case SaintDenisState::Combat: return "Combat";
        case SaintDenisState::Resolution: return "Resolution";
        case SaintDenisState::Cooldown: return "Cooldown";
        case SaintDenisState::Abort: return "Abort";
        case SaintDenisState::Cleanup: return "Cleanup";
        default: return "Unknown";
    }
}

} // namespace nightwalker::systems
