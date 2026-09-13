#include "nightwalker/narrative/NarrativeAudioManifest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace nightwalker::narrative {
namespace {

std::string Trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

void Diagnose(const NarrativeAudioDiagnosticSink& sink, const std::string& message) {
    if (sink) sink(message);
}

} // namespace

bool IsSafeNarrativeAudioAssetId(std::string_view assetId) noexcept {
    if (assetId.empty() || assetId.size() > 160) return false;
    for (const unsigned char c : assetId) {
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') continue;
        return false;
    }
    return true;
}

bool IsSafeNarrativeAudioRelativePath(const std::filesystem::path& path) noexcept {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
    for (const auto& part : path) {
        if (part == "..") return false;
    }
    auto extension = Lower(path.extension().string());
    return extension == ".wav";
}

bool NarrativeAudioManifest::Parse(std::string_view text, NarrativeAudioDiagnosticSink diagnostic) {
    Clear();
    std::istringstream input(std::string{text});
    std::string line;
    int schema = 1;
    bool schemaSeen = false;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        line = Trim(std::move(line));
        if (line.empty() || line.front() == '#') continue;

        if (line.rfind("schema=", 0) == 0) {
            if (schemaSeen) {
                Diagnose(diagnostic, "Nightwalker.audio contains more than one schema declaration.");
                Clear();
                return false;
            }
            schemaSeen = true;
            try {
                schema = std::stoi(Trim(line.substr(7)));
            } catch (...) {
                Diagnose(diagnostic, "Nightwalker.audio schema is not a valid integer.");
                Clear();
                return false;
            }
            if (schema != 1) {
                Diagnose(diagnostic, "Nightwalker.audio schema is unsupported; expected schema=1.");
                Clear();
                return false;
            }
            continue;
        }

        if (line.rfind("asset=", 0) != 0) {
            Diagnose(diagnostic, "Nightwalker.audio ignored an unknown line at " + std::to_string(lineNumber) + ".");
            continue;
        }

        const std::string payload = line.substr(6);
        const auto separator = payload.find('|');
        if (separator == std::string::npos) {
            Diagnose(diagnostic, "Nightwalker.audio asset entry is missing '|': line " + std::to_string(lineNumber) + ".");
            continue;
        }

        const std::string assetId = Trim(payload.substr(0, separator));
        const std::string relativeText = Trim(payload.substr(separator + 1));
        const std::filesystem::path relativePath(relativeText);
        if (!IsSafeNarrativeAudioAssetId(assetId)) {
            Diagnose(diagnostic, "Nightwalker.audio rejected an unsafe audio id at line " + std::to_string(lineNumber) + ".");
            continue;
        }
        if (!IsSafeNarrativeAudioRelativePath(relativePath)) {
            Diagnose(diagnostic, "Nightwalker.audio rejected an unsafe/non-WAV path at line " + std::to_string(lineNumber) + ".");
            continue;
        }
        if (assets_.contains(assetId)) {
            Diagnose(diagnostic, "Nightwalker.audio ignored a duplicate audio id: " + assetId + ".");
            continue;
        }
        assets_.emplace(assetId, relativePath.lexically_normal());
    }

    return true;
}

void NarrativeAudioManifest::Clear() noexcept {
    assets_.clear();
}

std::optional<std::filesystem::path> NarrativeAudioManifest::Resolve(
    std::string_view assetId,
    const std::filesystem::path& pluginDirectory) const {
    if (!IsSafeNarrativeAudioAssetId(assetId)) return std::nullopt;

    const auto found = assets_.find(std::string(assetId));
    if (found != assets_.end()) {
        return (pluginDirectory / found->second).lexically_normal();
    }

    std::filesystem::path fallback = pluginDirectory / L"audio";
    fallback /= std::string(assetId) + ".wav";
    return fallback.lexically_normal();
}

} // namespace nightwalker::narrative
