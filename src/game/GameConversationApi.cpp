#include "nightwalker/game/GameConversationApi.h"

#include <string>

#include "main.h"
#include "natives.h"

namespace nightwalker::game {

PromptHandle GameConversationApi::CreateEntityPrompt(EntityHandle entity, std::string_view controlName,
                                                     std::string_view label) noexcept {
    if (entity == 0 || controlName.empty() || label.empty()) return 0;
    const std::string control{controlName};
    const std::string text{label};
    const auto prompt = HUD::_UI_PROMPT_REGISTER_BEGIN();
    if (prompt == 0) return 0;
    HUD::_UI_PROMPT_SET_CONTROL_ACTION(prompt, MISC::GET_HASH_KEY(control.c_str()));
    HUD::_UI_PROMPT_SET_PRIORITY(prompt, 2);
    HUD::_UI_PROMPT_SET_TEXT(prompt, MISC::VAR_STRING(10, "LITERAL_STRING", text.c_str()));
    HUD::_UI_PROMPT_SET_STANDARD_MODE(prompt, TRUE);
    HUD::_UI_PROMPT_REGISTER_END(prompt);
    const int group = HUD::_UI_PROMPT_GET_GROUP_ID_FOR_TARGET_ENTITY(entity);
    if (group != 0) HUD::_UI_PROMPT_SET_GROUP(prompt, group, 0);
    HUD::_UI_PROMPT_SET_VISIBLE(prompt, FALSE);
    HUD::_UI_PROMPT_SET_ENABLED(prompt, FALSE);
    return static_cast<PromptHandle>(prompt);
}

void GameConversationApi::SetPromptEnabled(PromptHandle prompt, bool enabled) noexcept {
    if (prompt == 0) return;
    HUD::_UI_PROMPT_SET_ENABLED(prompt, enabled ? TRUE : FALSE);
    HUD::_UI_PROMPT_SET_VISIBLE(prompt, enabled ? TRUE : FALSE);
}

bool GameConversationApi::PromptActivated(PromptHandle prompt) const noexcept {
    return prompt != 0 && HUD::_UI_PROMPT_HAS_STANDARD_MODE_COMPLETED(prompt, 0) == TRUE;
}

void GameConversationApi::DeletePrompt(PromptHandle& prompt) noexcept {
    if (prompt == 0) return;
    HUD::_UI_PROMPT_SET_VISIBLE(prompt, FALSE);
    HUD::_UI_PROMPT_SET_ENABLED(prompt, FALSE);
    if (HUD::_UI_PROMPT_IS_VALID(prompt) == TRUE) HUD::_UI_PROMPT_DELETE(prompt);
    prompt = 0;
}

bool GameConversationApi::IsPedShooting(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_SHOOTING(ped) == TRUE;
}

} // namespace nightwalker::game
