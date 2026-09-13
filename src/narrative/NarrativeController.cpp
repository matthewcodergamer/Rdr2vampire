#include "nightwalker/narrative/NarrativeController.h"

#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace nightwalker::narrative {
namespace {

enum class CatalogLoadResult { Missing, Loaded, Rejected };

CatalogLoadResult ReadCatalog(const std::filesystem::path& path,
                              NarrativeCatalog& output,
                              util::Logger& logger,
                              std::string_view label) {
    std::ifstream input(path);
    if (!input) return CatalogLoadResult::Missing;
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!ParseNarrativeScript(buffer.str(), output, [&logger, label](std::string_view message) {
            logger.Write(util::LogLevel::Warning,
                         std::string(label) + ": " + std::string(message));
        })) {
        logger.Write(util::LogLevel::Warning,
                     std::string(label) + " rejected; existing dialogue remains active.");
        return CatalogLoadResult::Rejected;
    }
    return CatalogLoadResult::Loaded;
}

void AppendSupplement(NarrativeCatalog& destination,
                      NarrativeCatalog&& supplement,
                      util::Logger& logger) {
    std::size_t appended = 0;
    for (auto& sequence : supplement.sequences) {
        if (destination.Find(sequence.id) != nullptr) {
            logger.Write(util::LogLevel::Warning,
                         "Nightwalker.voice.dialogue ignored duplicate sequence: " + sequence.id);
            continue;
        }
        destination.sequences.push_back(std::move(sequence));
        ++appended;
    }
    logger.Write(util::LogLevel::Info,
                 "Nightwalker.voice.dialogue loaded: " + std::to_string(appended) +
                     " supplemental sequence(s).");
}

} // namespace

NarrativeController::NarrativeController(game::IGameBossBarApi& textApi,
                                         game::IGameNarrativeAudioApi& audioApi,
                                         util::Logger& logger,
                                         const core::Config& config) noexcept
    : textApi_(textApi), audioApi_(audioApi), logger_(logger), config_(config) {}

bool NarrativeController::Initialize() {
    if (catalog_.Empty()) catalog_ = BuiltInNarrativeCatalog();
    variants_.Reset();
    variationNonce_ = 0;
    Cancel();
    logger_.Write(util::LogLevel::Info,
                  "NarrativeController initialized; coherent sequence families use a non-repeating shuffle bag and optional audio cannot block gameplay.");
    return true;
}

void NarrativeController::LoadScript(const std::filesystem::path& path) {
    Cancel();
    scriptPath_ = path;
    catalog_ = BuiltInNarrativeCatalog();
    variants_.Reset();
    variationNonce_ = 0;
    try {
        NarrativeCatalog primary{};
        const auto primaryResult = ReadCatalog(path, primary, logger_, "Nightwalker.dialogue");
        if (primaryResult == CatalogLoadResult::Loaded) {
            catalog_ = std::move(primary);
            logger_.Write(util::LogLevel::Info, "Nightwalker.dialogue loaded successfully.");
        } else if (primaryResult == CatalogLoadResult::Missing) {
            logger_.Write(util::LogLevel::Info,
                          "Nightwalker.dialogue not found/readable; built-in original subtitles are active.");
        }

        NarrativeCatalog supplement{};
        const auto supplementPath = path.parent_path() / L"Nightwalker.voice.dialogue";
        if (ReadCatalog(supplementPath, supplement, logger_,
                        "Nightwalker.voice.dialogue") == CatalogLoadResult::Loaded) {
            AppendSupplement(catalog_, std::move(supplement), logger_);
        }
    } catch (...) {
        logger_.Write(util::LogLevel::Error,
                      "Narrative source load failed; built-in/original dialogue remains active.");
    }
}

bool NarrativeController::StartResolvedSequence(const NarrativeSequence& sequence,
                                                std::uint64_t nowMs) noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::Narrative)) return false;
    audioApi_.Stop();
    presentedLineId_.clear();
    if (!playback_.Start(sequence, nowMs,
                         static_cast<std::uint64_t>(config_.narrative.maxSequenceMs))) return false;
    skipWasDown_ = SkipKeyDown();
    PresentAudioForCurrentLine();
    if (config_.debug.enabled) {
        logger_.Write(util::LogLevel::Debug,
                      std::string("Narrative sequence started: ") + sequence.id);
    }
    return true;
}

bool NarrativeController::StartSequence(std::string_view sequenceId,
                                        std::uint64_t nowMs) noexcept {
    const auto* sequence = catalog_.Find(sequenceId);
    if (!sequence) {
        logger_.Write(util::LogLevel::Warning,
                      std::string("Narrative sequence missing: ") + std::string(sequenceId));
        return false;
    }
    return StartResolvedSequence(*sequence, nowMs);
}

bool NarrativeController::StartSequenceFamily(std::string_view familyId,
                                              std::uint64_t nowMs) noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::Narrative)) return false;
    ++variationNonce_;
    const auto entropy = nowMs ^ (variationNonce_ * 0x9E3779B97F4A7C15ULL);
    const auto* sequence = variants_.Choose(catalog_, familyId, entropy);
    if (!sequence) {
        logger_.Write(util::LogLevel::Warning,
                      std::string("Narrative sequence family missing: ") + std::string(familyId));
        return false;
    }
    return StartResolvedSequence(*sequence, nowMs);
}

void NarrativeController::Update(const core::FrameContext& frame) {
    if (!config_.IsFeatureEnabled(core::Feature::Narrative)) { Cancel(); return; }
    if (!playback_.Active()) { skipWasDown_ = SkipKeyDown(); return; }
    const bool down = SkipKeyDown();
    if (down && !skipWasDown_) RequestSkip(frame.nowMs);
    skipWasDown_ = down;
    const std::string before = playback_.CurrentLine() ? playback_.CurrentLine()->id : std::string{};
    playback_.Update(frame.nowMs);
    if (!playback_.Active()) { audioApi_.Stop(); presentedLineId_.clear(); return; }
    const auto* current = playback_.CurrentLine();
    if (current && current->id != before) PresentAudioForCurrentLine();
    DrawCurrentSubtitle();
}

void NarrativeController::RequestSkip(std::uint64_t nowMs) noexcept {
    if (!playback_.Active()) return;
    audioApi_.Stop();
    presentedLineId_.clear();
    playback_.Skip(nowMs);
    if (playback_.Active()) PresentAudioForCurrentLine();
}

void NarrativeController::Cancel() noexcept {
    if (playback_.Active() && config_.debug.enabled)
        logger_.Write(util::LogLevel::Debug, "Narrative sequence cancelled.");
    audioApi_.Stop();
    playback_.Cancel();
    presentedLineId_.clear();
    skipWasDown_ = false;
}

void NarrativeController::Shutdown() noexcept {
    Cancel();
    variants_.Reset();
    variationNonce_ = 0;
    catalog_ = {};
    scriptPath_.clear();
}

bool NarrativeController::SkipKeyDown() const noexcept {
    const int key = config_.narrative.skipKey;
    return key > 0 && key <= 255 && (::GetAsyncKeyState(key) & 0x8000) != 0;
}

void NarrativeController::PresentAudioForCurrentLine() noexcept {
    const auto* line = playback_.CurrentLine();
    if (!line || line->id == presentedLineId_) return;
    presentedLineId_ = line->id;
    if (!config_.narrative.optionalAudio || line->audioId.empty()) return;
    if (!audioApi_.TryPlay(line->audioId) && config_.debug.enabled)
        logger_.Write(util::LogLevel::Debug,
                      std::string("Optional narrative audio unavailable; subtitle fallback active for ") +
                          line->audioId);
}

void NarrativeController::DrawCurrentSubtitle() noexcept {
    const auto* line = playback_.CurrentLine();
    if (!line) return;
    auto wrapped = WrapSubtitle(line->text, 68, 3);
    if (wrapped.empty()) return;
    int width = 1920, height = 1080;
    textApi_.Resolution(width, height);
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                                    : 16.0F / 9.0F;
    const float scale = (16.0F / 9.0F) / std::max(1.0F, aspect);
    const float boxWidth = std::clamp(0.72F * scale, 0.46F, 0.74F);
    const bool speaker = !line->speaker.empty();
    const float top = 0.705F, lineStep = 0.034F;
    const float contentLines = static_cast<float>(wrapped.size() + (speaker ? 1 : 0));
    const float boxHeight = 0.026F + contentLines * lineStep;
    float y = top + 0.006F;
    textApi_.Rectangle(0.5F, top + boxHeight * 0.5F, boxWidth, boxHeight, 8, 8, 8, 118);
    if (speaker) {
        textApi_.CenteredText(line->speaker.c_str(), 0.5F, y, 0.31F, 216, 208, 194, 240);
        y += lineStep;
    }
    for (const auto& text : wrapped) {
        textApi_.CenteredText(text.c_str(), 0.5F, y, 0.37F, 235, 231, 220, 245);
        y += lineStep;
    }
}

} // namespace nightwalker::narrative
