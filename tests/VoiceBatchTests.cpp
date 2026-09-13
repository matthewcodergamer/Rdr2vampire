#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "nightwalker/narrative/NarrativeAudioManifest.h"
#include "nightwalker/narrative/NarrativeScript.h"

namespace {
std::string ReadAll(const char* path) {
    std::ifstream input(path);
    assert(input && "required voice library file is missing");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

int main() {
    using namespace nightwalker::narrative;

    std::vector<std::string> warnings;
    NarrativeCatalog catalog{};
    assert(ParseNarrativeScript(ReadAll("content/Nightwalker.dialogue"), catalog,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());

    NarrativeCatalog supplement{};
    assert(ParseNarrativeScript(ReadAll("Nightwalker.voice.dialogue"), supplement,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    for (auto& sequence : supplement.sequences) {
        assert(catalog.Find(sequence.id) == nullptr);
        catalog.sequences.push_back(std::move(sequence));
    }

    const auto* soul03 = catalog.Find("saint_denis.pre_fight.soul_03");
    const auto* soul05 = catalog.Find("saint_denis.pre_fight.soul_05");
    const auto* soul08 = catalog.Find("saint_denis.pre_fight.soul_08");
    const auto* recordedChallenge = catalog.Find("saint_denis.choice.challenge.recorded_01");
    const auto* secondLoweredTake = catalog.Find("saint_denis.react.lowered.04");
    assert(soul03 && soul03->lines.size() == 4);
    assert(soul05 && soul05->lines.size() == 4);
    assert(soul08 && soul08->lines.size() == 4);
    assert(recordedChallenge && recordedChallenge->lines.size() == 1);
    assert(secondLoweredTake && secondLoweredTake->lines.size() == 1);

    NarrativeAudioManifest manifest{};
    warnings.clear();
    assert(manifest.Parse(ReadAll("content/Nightwalker.audio"),
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    assert(manifest.Count() == 67);

    std::unordered_set<std::string> physicalPaths;
    for (const auto& sequence : catalog.sequences) {
        for (const auto& line : sequence.lines) {
            if (line.audioId.empty()) continue;
            const auto installedPath = manifest.Resolve(line.audioId, "install-root");
            if (!installedPath) continue;
            assert(installedPath->parent_path().filename() == "audio");
            assert(installedPath->extension() == ".mp3" || installedPath->extension() == ".wav");
            const auto sourcePath = std::filesystem::path("content") / installedPath->filename();
            if (!std::filesystem::is_regular_file(sourcePath)) continue; // subtitle-only fallback.
            physicalPaths.insert(sourcePath.generic_string());
        }
    }
    assert(physicalPaths.size() == 64);

    const auto preFight = catalog.FindFamily("saint_denis.pre_fight");
    const auto talk = catalog.FindFamily("saint_denis.choice.question");
    const auto antagonize = catalog.FindFamily("saint_denis.choice.challenge");
    const auto leave = catalog.FindFamily("saint_denis.choice.leave");
    assert(preFight.size() >= 11);
    assert(talk.size() >= 8);
    assert(antagonize.size() >= 4);
    assert(leave.size() >= 6);

    NarrativeVariantSelector selector{};
    std::unordered_set<std::string> chosen;
    for (std::uint64_t entropy = 0; entropy < preFight.size(); ++entropy) {
        const auto* selected = selector.Choose(catalog, "saint_denis.pre_fight", entropy);
        assert(selected);
        chosen.insert(selected->id);
    }
    assert(chosen.size() == preFight.size());
    const auto* afterRefill = selector.Choose(catalog, "saint_denis.pre_fight", 9999);
    assert(afterRefill);

    return 0;
}
