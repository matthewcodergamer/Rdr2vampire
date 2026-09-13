#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nightwalker::narrative {

using NarrativeAudioDiagnosticSink = std::function<void(std::string_view)>;

inline std::string TrimNarrativeAudioText(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

inline std::string LowerNarrativeAudioText(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

[[nodiscard]] inline bool IsSafeNarrativeAudioAssetId(std::string_view assetId) noexcept {
    if (assetId.empty() || assetId.size() > 160) return false;
    for (const unsigned char c : assetId) {
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') continue;
        return false;
    }
    return true;
}

[[nodiscard]] inline bool IsSafeNarrativeAudioRelativePath(const std::filesystem::path& path) noexcept {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
    for (const auto& part : path) {
        if (part == "..") return false;
    }
    const auto extension = LowerNarrativeAudioText(path.extension().string());
    return extension == ".wav" || extension == ".mp3";
}

class NarrativeAudioManifest final {
public:
    bool Parse(std::string_view text, NarrativeAudioDiagnosticSink diagnostic = {}) {
        Clear();
        std::istringstream input(std::string{text});
        std::string line;
        bool schemaSeen = false;
        std::size_t lineNumber = 0;

        const auto diagnose = [&diagnostic](const std::string& message) {
            if (diagnostic) diagnostic(message);
        };

        while (std::getline(input, line)) {
            ++lineNumber;
            line = TrimNarrativeAudioText(std::move(line));
            if (line.empty() || line.front() == '#') continue;

            if (line.rfind("schema=", 0) == 0) {
                if (schemaSeen) {
                    diagnose("Nightwalker.audio contains more than one schema declaration.");
                    Clear();
                    return false;
                }
                schemaSeen = true;
                try {
                    if (std::stoi(TrimNarrativeAudioText(line.substr(7))) != 1) {
                        diagnose("Nightwalker.audio schema is unsupported; expected schema=1.");
                        Clear();
                        return false;
                    }
                } catch (...) {
                    diagnose("Nightwalker.audio schema is not a valid integer.");
                    Clear();
                    return false;
                }
                continue;
            }

            if (line.rfind("asset=", 0) != 0) {
                diagnose("Nightwalker.audio ignored an unknown line at " + std::to_string(lineNumber) + ".");
                continue;
            }

            const std::string payload = line.substr(6);
            const auto separator = payload.find('|');
            if (separator == std::string::npos) {
                diagnose("Nightwalker.audio asset entry is missing '|': line " + std::to_string(lineNumber) + ".");
                continue;
            }

            const std::string assetId = TrimNarrativeAudioText(payload.substr(0, separator));
            const std::filesystem::path relativePath(TrimNarrativeAudioText(payload.substr(separator + 1)));
            if (!IsSafeNarrativeAudioAssetId(assetId)) {
                diagnose("Nightwalker.audio rejected an unsafe audio id at line " + std::to_string(lineNumber) + ".");
                continue;
            }
            if (!IsSafeNarrativeAudioRelativePath(relativePath)) {
                diagnose("Nightwalker.audio rejected an unsafe/unsupported audio path at line " + std::to_string(lineNumber) + ".");
                continue;
            }
            if (assets_.contains(assetId)) {
                diagnose("Nightwalker.audio ignored a duplicate audio id: " + assetId + ".");
                continue;
            }
            assets_.emplace(assetId, relativePath.lexically_normal());
        }
        return true;
    }

    void Clear() noexcept { assets_.clear(); }

    [[nodiscard]] std::optional<std::filesystem::path> Resolve(
        std::string_view assetId,
        const std::filesystem::path& pluginDirectory) const {
        if (!IsSafeNarrativeAudioAssetId(assetId)) return std::nullopt;
        const auto found = assets_.find(std::string(assetId));
        if (found != assets_.end()) return (pluginDirectory / found->second).lexically_normal();
        std::filesystem::path fallback = pluginDirectory / L"audio";
        fallback /= std::string(assetId) + ".wav";
        return fallback.lexically_normal();
    }

    [[nodiscard]] std::size_t Count() const noexcept { return assets_.size(); }

private:
    std::unordered_map<std::string, std::filesystem::path> assets_{};
};

} // namespace nightwalker::narrative
