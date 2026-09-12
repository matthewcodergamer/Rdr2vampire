#include "nightwalker/game/GameBossBarApi.h"

#include <algorithm>

#include <natives.h>

namespace nightwalker::game {

void GameBossBarApi::Resolution(int& width, int& height) const noexcept {
    width = 1920;
    height = 1080;
    GRAPHICS::GET_SCREEN_RESOLUTION(&width, &height);
    if (width <= 0 || height <= 0) {
        width = 1920;
        height = 1080;
    }
}

void GameBossBarApi::Rectangle(float x, float y, float width, float height,
                               int red, int green, int blue, int alpha) noexcept {
    GRAPHICS::DRAW_RECT(x, y, width, height,
                        std::clamp(red, 0, 255), std::clamp(green, 0, 255),
                        std::clamp(blue, 0, 255), std::clamp(alpha, 0, 255),
                        TRUE, FALSE);
}

void GameBossBarApi::CenteredText(const char* text, float x, float y, float scale,
                                  int red, int green, int blue, int alpha) noexcept {
    if (text == nullptr || text[0] == '\0') return;
    const char* value = MISC::VAR_STRING(10, "LITERAL_STRING", text);
    if (value == nullptr) return;

    UIDEBUG::_BG_SET_TEXT_SCALE(scale, scale);
    UIDEBUG::_BG_SET_TEXT_COLOR(
        std::clamp(red, 0, 255), std::clamp(green, 0, 255),
        std::clamp(blue, 0, 255), std::clamp(alpha, 0, 255));
    HUD::SET_TEXT_CENTRE(TRUE);
    UIDEBUG::_BG_DISPLAY_TEXT(value, x, y);
    HUD::SET_TEXT_CENTRE(FALSE);
}

} // namespace nightwalker::game
