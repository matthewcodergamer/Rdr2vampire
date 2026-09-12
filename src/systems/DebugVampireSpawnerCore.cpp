#include "nightwalker/systems/DebugVampireSpawner.h"

#include <string>

namespace nightwalker::systems {

DebugVampireSpawner::DebugVampireSpawner(
    game::IGameApi& api, BossActorRegistry& registry, util::Logger& logger,
    const core::Config& config) noexcept
    : api_(api), registry_(registry), logger_(logger), config_(config) {}

bool DebugVampireSpawner::Initialize() {
    ownedPed_ = 0;
    state_ = State::Idle;
    return true;
}

void DebugVampireSpawner::RequestSpawn(std::uint64_t nowMs) noexcept {
    if (!config_.debug.enabled) return;
    if (registry_.Ped() != 0) {
        if (registry_.Owner() == BossOwner::Debug && registry_.Ped() == ownedPed_) {
            logger_.Write(util::LogLevel::Debug, "Debug vampire already exists; duplicate spawn ignored.");
        } else {
            logger_.Write(util::LogLevel::Debug,
                "Debug vampire spawn ignored because another boss owner is active.");
        }
        return;
    }
    if (state_ == State::LoadingModel) return;
    ownedPed_ = 0;
    const auto status = modelRequest_.Begin(
        api_, kVampireModel, nowMs, static_cast<std::uint64_t>(config_.debug.modelLoadTimeoutMs));
    if (status == game::ModelStreamStatus::InvalidModel) {
        logger_.Write(util::LogLevel::Error, "cs_vampire model is unavailable or invalid.");
        ResetRequest();
        return;
    }
    state_ = State::LoadingModel;
}

void DebugVampireSpawner::Update(const core::FrameContext& frame) {
    if (state_ == State::Spawned) {
        if (ownedPed_ != 0 && (!api_.EntityExists(ownedPed_) || api_.EntityModel(ownedPed_) != kVampireModel)) {
            registry_.Release(ownedPed_, BossOwner::Debug);
            ownedPed_ = 0;
            state_ = State::Idle;
        }
        return;
    }
    if (state_ != State::LoadingModel) return;
    if (registry_.Ped() != 0) {
        ResetRequest();
        return;
    }
    const auto status = modelRequest_.Update(api_, frame.nowMs);
    if (status == game::ModelStreamStatus::TimedOut) {
        logger_.Write(util::LogLevel::Error,
            std::string("cs_vampire model request timed out after ") +
            std::to_string(frame.nowMs - modelRequest_.StartedAtMs()) + " ms.");
        ResetRequest();
        return;
    }
    if (status == game::ModelStreamStatus::Loaded) SpawnLoadedModel(frame.nowMs);
}

void DebugVampireSpawner::ResetRequest() noexcept {
    modelRequest_.Release(api_);
    if (state_ != State::Spawned) state_ = State::Idle;
}

} // namespace nightwalker::systems
