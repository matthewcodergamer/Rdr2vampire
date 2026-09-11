#pragma once

#include <Windows.h>
#include <cstdint>
#include "nightwalker/game/GameContext.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::core {
class Runtime final {
public:
    Runtime() = default;
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    [[nodiscard]] bool Initialize(HMODULE moduleHandle);
    void Tick();
    void Shutdown();
    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
private:
    game::GameContext gameContext_{};
    util::Logger logger_{};
    std::uint64_t tickCount_{0};
    bool initialized_{false};
};
}
