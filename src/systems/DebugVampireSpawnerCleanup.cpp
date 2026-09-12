#include "nightwalker/systems/DebugVampireSpawner.h"

#include <string>

namespace nightwalker::systems {

void DebugVampireSpawner::RequestDespawn() noexcept {
    if (state_ == State::LoadingModel) {
        modelRequest_.Release(api_);
        state_ = State::Idle;
    }
    if (ownedPed_ == 0) return;
    if (registry_.Ped() != ownedPed_ || registry_.Owner() != BossOwner::Debug) {
        ownedPed_ = 0;
        state_ = State::Idle;
        return;
    }
    if (!api_.EntityExists(ownedPed_)) {
        registry_.Release(ownedPed_, BossOwner::Debug);
        ownedPed_ = 0;
        state_ = State::Idle;
        return;
    }
    if (api_.EntityModel(ownedPed_) != kVampireModel) {
        logger_.Write(util::LogLevel::Error,
            "Refusing to delete stale debug handle because its model no longer matches cs_vampire.");
        registry_.Release(ownedPed_, BossOwner::Debug);
        ownedPed_ = 0;
        state_ = State::Idle;
        return;
    }

    registry_.SetCombatEnabled(ownedPed_, BossOwner::Debug, false);
    game::PedHandle handle = ownedPed_;
    if (api_.DeletePed(handle)) {
        registry_.Release(ownedPed_, BossOwner::Debug);
        logger_.Write(util::LogLevel::Info,
            std::string("Cleaned up debug vampire handle=") + std::to_string(ownedPed_));
        ownedPed_ = 0;
        state_ = State::Idle;
    } else {
        ownedPed_ = handle;
        registry_.SetCombatEnabled(ownedPed_, BossOwner::Debug, true);
        logger_.Write(util::LogLevel::Error,
            std::string("Failed to delete debug vampire handle=") + std::to_string(ownedPed_));
    }
}

void DebugVampireSpawner::Cancel() noexcept { RequestDespawn(); }
void DebugVampireSpawner::Shutdown() noexcept { RequestDespawn(); ResetRequest(); }

} // namespace nightwalker::systems
