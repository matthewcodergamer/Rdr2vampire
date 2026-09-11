#pragma once

#include <cstdint>
#include <string_view>

namespace nightwalker::core {

struct FrameContext final {
    std::uint64_t nowMs{0};
    double deltaSeconds{0.0};
};

class ILifecycleSystem {
public:
    virtual ~ILifecycleSystem() = default;
    virtual std::string_view Name() const noexcept = 0;
    virtual bool Initialize() = 0;
    virtual void Update(const FrameContext& frame) = 0;
    virtual void Cancel() noexcept = 0;
    virtual void Shutdown() noexcept = 0;
};

}