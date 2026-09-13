#pragma once

#include <string_view>

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

using PromptHandle = int;

class IGameConversationApi {
public:
    virtual ~IGameConversationApi() = default;
    virtual PromptHandle CreateEntityPrompt(EntityHandle entity, std::string_view controlName,
                                            std::string_view label) noexcept = 0;
    virtual void SetPromptEnabled(PromptHandle prompt, bool enabled) noexcept = 0;
    [[nodiscard]] virtual bool PromptActivated(PromptHandle prompt) const noexcept = 0;
    virtual void DeletePrompt(PromptHandle& prompt) noexcept = 0;
    [[nodiscard]] virtual bool IsPedShooting(PedHandle ped) const noexcept = 0;
};

class GameConversationApi final : public IGameConversationApi {
public:
    PromptHandle CreateEntityPrompt(EntityHandle entity, std::string_view controlName,
                                    std::string_view label) noexcept override;
    void SetPromptEnabled(PromptHandle prompt, bool enabled) noexcept override;
    [[nodiscard]] bool PromptActivated(PromptHandle prompt) const noexcept override;
    void DeletePrompt(PromptHandle& prompt) noexcept override;
    [[nodiscard]] bool IsPedShooting(PedHandle ped) const noexcept override;
};

} // namespace nightwalker::game
