#include "nightwalker/game/GameBossBarApi.h"

#include <algorithm>
#include <string>

#include <natives.h>

namespace nightwalker::game {
namespace {
std::string EscapeText(std::string value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}
}

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

    // RDR2's current text path uses inline formatting for alignment. The gameplay
    // coordinate is normalized 0..1; centered formatted text expects -1..1 X.
    const std::string formatted =
        "<TEXTFORMAT RIGHTMARGIN='0'><P ALIGN='Center'>" +
        EscapeText(text) + "</P><TEXTFORMAT>";
    const char* value = MISC::VAR_STRING(10, "LITERAL_STRING", formatted.c_str());
    if (value == nullptr) return;

    UIDEBUG::_BG_SET_TEXT_SCALE(scale, scale);
    UIDEBUG::_BG_SET_TEXT_COLOR(
        std::clamp(red, 0, 255), std::clamp(green, 0, 255),
        std::clamp(blue, 0, 255), std::clamp(alpha, 0, 255));
    UIDEBUG::_BG_DISPLAY_TEXT(value, -1.0F + (std::clamp(x, 0.0F, 1.0F) * 2.0F), y);
}

} // namespace nightwalker::game
