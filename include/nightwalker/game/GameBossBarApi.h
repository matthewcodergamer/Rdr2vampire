#pragma once

namespace nightwalker::game {

class IGameBossBarApi {
public:
    virtual ~IGameBossBarApi() = default;
    virtual void Resolution(int& width, int& height) const noexcept = 0;
    virtual void Rectangle(float x, float y, float width, float height,
                           int red, int green, int blue, int alpha) noexcept = 0;
    virtual void CenteredText(const char* text, float x, float y, float scale,
                              int red, int green, int blue, int alpha) noexcept = 0;
};

class GameBossBarApi final : public IGameBossBarApi {
public:
    void Resolution(int& width, int& height) const noexcept override;
    void Rectangle(float x, float y, float width, float height,
                   int red, int green, int blue, int alpha) noexcept override;
    void CenteredText(const char* text, float x, float y, float scale,
                      int red, int green, int blue, int alpha) noexcept override;
};

} // namespace nightwalker::game
