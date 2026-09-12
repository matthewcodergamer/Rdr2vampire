#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameBossBarApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameFeedingApi.h"
#include "nightwalker/game/GamePhysicalApi.h"
#include "nightwalker/ui/BossHudModel.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::ui {

class BossHudController final : public core::ILifecycleSystem {
public:
    BossHudController(game::IGameApi& api, game::IGameCombatApi& combatApi,
        game::IGameFeedingApi& feedingApi, game::IGamePhysicalApi& physicalApi,
        game::IGameBossBarApi& drawApi, util::Logger& logger,
        const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "BossHudController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    bool BeginBoss(game::PedHandle boss, std::string_view displayName,
                   std::uint64_t nowMs) noexcept;
    void NotifyCombatActivity(std::uint64_t nowMs) noexcept;
    void EndBoss(bool defeated, std::uint64_t nowMs) noexcept;
    void ForceHide() noexcept;

    [[nodiscard]] game::PedHandle ActiveBoss() const noexcept { return boss_; }
    [[nodiscard]] BossHudState State() const noexcept { return model_.State(); }
    [[nodiscard]] float ActualHealthRatio() const noexcept { return model_.ActualHealthRatio(); }
    [[nodiscard]] float DisplayHealthRatio() const noexcept { return model_.DisplayHealthRatio(); }

private:
    [[nodiscard]] float ReadBossHealthRatio(int& health, int& maxHealth) const noexcept;
    [[nodiscard]] bool CloseCombatEngagement(game::PedHandle player) const noexcept;
    void Draw() noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGameFeedingApi& feedingApi_;
    game::IGamePhysicalApi& physicalApi_;
    game::IGameBossBarApi& drawApi_;
    util::Logger& logger_;
    const core::Config& config_;
    BossHudModel model_{};
    game::PedHandle boss_{0};
    std::string displayName_{"THE VAMPIRE"};
    int bossHealth_{0};
    int bossMaxHealth_{1};
    int playerHealth_{0};
};

} // namespace nightwalker::ui
