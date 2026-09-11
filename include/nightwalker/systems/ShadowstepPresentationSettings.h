#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
namespace nightwalker::systems {
struct ShadowstepPresentationSettings final {
    std::uint32_t disappearMs{110};
    float carryMeters{1.25F};
    std::uint32_t carryMs{140};
    std::uint32_t meleeBufferMs{220};
    std::uint32_t stateTimeoutMs{1000};
    bool smokeFx{true};
};
using PresentationDiagnosticSink = std::function<void(std::string_view)>;
ShadowstepPresentationSettings LoadShadowstepPresentationSettings(
    const std::filesystem::path& path, PresentationDiagnosticSink diagnostics = {});
}
