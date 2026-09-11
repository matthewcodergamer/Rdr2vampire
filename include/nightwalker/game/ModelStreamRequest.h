#pragma once
#include <cstdint>
#include "nightwalker/game/GameApi.h"
namespace nightwalker::game {
enum class ModelStreamStatus { Idle, Loading, Loaded, InvalidModel, TimedOut };
class ModelStreamRequest final {
public:
 ModelStreamStatus Begin(IGameApi& api, ModelHash model, std::uint64_t nowMs, std::uint64_t timeoutMs) noexcept;
 ModelStreamStatus Update(IGameApi& api, std::uint64_t nowMs) noexcept;
 void Release(IGameApi& api) noexcept;
 ModelStreamStatus Status() const noexcept { return status_; }
 ModelHash Model() const noexcept { return model_; }
 std::uint64_t StartedAtMs() const noexcept { return startedAtMs_; }
private:
 ModelHash model_{0};
 std::uint64_t startedAtMs_{0};
 std::uint64_t timeoutMs_{0};
 ModelStreamStatus status_{ModelStreamStatus::Idle};
};
}
