#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/SaveData.h"
#include "nightwalker/game/GameEncounterApi.h"
#include "nightwalker/systems/FeedingController.h"
#include "nightwalker/systems/SaintDenisDirector.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

class ProgressionController final : public core::ILifecycleSystem {
public:
    ProgressionController(core::Config& config,
                          game::IGameEncounterApi& encounterApi,
                          FeedingController& feeding,
                          SaintDenisDirector& encounter,
                          util::Logger& logger) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "ProgressionController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    void LoadBeforeSystems(const std::filesystem::path& path);
    void ApplyAfterConfigReload() noexcept;
    [[nodiscard]] const core::NightwalkerSaveData& Data() const noexcept { return data_; }

private:
    void RefreshEncounterGate() noexcept;
    void Capture() noexcept;
    void Checkpoint(bool force) noexcept;

    core::Config& config_;
    game::IGameEncounterApi& encounterApi_;
    FeedingController& feeding_;
    SaintDenisDirector& encounter_;
    util::Logger& logger_;
    core::NightwalkerSaveData data_{};
    std::filesystem::path path_{};
    std::string lastSnapshot_{};
    std::uint64_t nextCheckMs_{0};
    bool writeAllowed_{true};
    bool encounterGateActive_{false};
    bool baseEncounterEnabled_{true};
};

} // namespace nightwalker::systems
