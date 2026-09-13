#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nightwalker::narrative {

using NarrativeAudioDiagnosticSink = std::function<void(std::string_view)>;

class NarrativeAudioManifest final {
public:
    bool Parse(std::string_view text, NarrativeAudioDiagnosticSink diagnostic = {});
    void Clear() noexcept;

    [[nodiscard]] std::optional<std::filesystem::path> Resolve(
        std::string_view assetId,
        const std::filesystem::path& pluginDirectory) const;

    [[nodiscard]] std::size_t Count() const noexcept { return assets_.size(); }

private:
    std::unordered_map<std::string, std::filesystem::path> assets_{};
};

[[nodiscard]] bool IsSafeNarrativeAudioAssetId(std::string_view assetId) noexcept;
[[nodiscard]] bool IsSafeNarrativeAudioRelativePath(const std::filesystem::path& path) noexcept;

} // namespace nightwalker::narrative
