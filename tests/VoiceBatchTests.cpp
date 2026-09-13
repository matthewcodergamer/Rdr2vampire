#include <cassert>
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
    assert(input && "required voice batch fixture is missing");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

int main() {
    using namespace nightwalker::narrative;

    std::vector<std::string> warnings;
    NarrativeCatalog base{};
    assert(ParseNarrativeScript(ReadAll("content/Nightwalker.dialogue"), base,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());

    warnings.clear();
    NarrativeCatalog supplement{};
    assert(ParseNarrativeScript(ReadAll("Nightwalker.voice.dialogue"), supplement,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    assert(supplement.sequences.size() == 7);

    const auto* soul = supplement.Find("saint_denis.pre_fight.recorded_soul_01");
    const auto* city = supplement.Find("saint_denis.choice.question.city_01");
    const auto* leaveA = supplement.Find("saint_denis.choice.leave.rare_wisdom_01");
    const auto* leaveB = supplement.Find("saint_denis.choice.leave.rare_wisdom_02");
    const auto* leaveC = supplement.Find("saint_denis.choice.leave.rare_wisdom_03");
    const auto* aimHold = supplement.Find("saint_denis.react.aim_hold.care_01");
    const auto* church = supplement.Find("saint_denis.choice.question.church_01");

    assert(soul && soul->lines.size() == 1);
    assert(city && city->lines.size() == 3);
    assert(leaveA && leaveA->lines.size() == 1);
    assert(leaveB && leaveB->lines.size() == 1);
    assert(leaveC && leaveC->lines.size() == 1);
    assert(aimHold && aimHold->lines.size() == 1);
    assert(church && church->lines.size() == 2);

    std::unordered_set<std::string> sequenceIds;
    for (const auto& sequence : base.sequences) sequenceIds.insert(sequence.id);
    for (auto& sequence : supplement.sequences) {
        assert(sequenceIds.insert(sequence.id).second);
        base.sequences.push_back(std::move(sequence));
    }

    assert(base.FindFamily(ids::kSaintDenisPreFight).size() >= 11);
    assert(base.FindFamily("saint_denis.choice.question").size() >= 8);
    assert(base.FindFamily("saint_denis.choice.leave").size() >= 6);
    assert(base.FindFamily("saint_denis.react.aim_hold").size() >= 4);

    NarrativeAudioManifest manifest{};
    warnings.clear();
    assert(manifest.Parse(ReadAll("content/Nightwalker.audio"),
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    assert(manifest.Count() == 26);

    const std::vector<std::string> ids{
        "nw.audio.sd.first_contact.nearer.01",
        "nw.audio.sd.first_contact.wiser.01",
        "nw.audio.sd.question.names.01",
        "nw.audio.sd.soul.recorded.01",
        "nw.audio.sd.question.city.01a",
        "nw.audio.sd.leave.rare_wisdom.01",
        "nw.audio.sd.aimhold.care.01",
        "nw.audio.sd.question.church.01a",
        "nw.audio.sd.soul.01a",
        "nw.audio.sd.soul.01b",
        "nw.audio.sd.soul.02a",
        "nw.audio.sd.soul.03a",
        "nw.audio.sd.soul.03b",
        "nw.audio.sd.soul.04a",
    };
    for (const auto& id : ids) {
        const auto path = manifest.Resolve(id, "NightwalkerRoot");
        assert(path.has_value());
        assert(path->extension() == ".mp3");
    }

    const auto soulAlias = manifest.Resolve("nw.audio.sd.soul.01a", "NightwalkerRoot");
    const auto recordedSoul = manifest.Resolve("nw.audio.sd.soul.recorded.01", "NightwalkerRoot");
    assert(soulAlias.has_value() && recordedSoul.has_value());
    assert(soulAlias->filename() == recordedSoul->filename());

    NarrativeVariantSelector leaveSelector{};
    std::unordered_set<std::string> chosen;
    const auto leaveFamily = base.FindFamily("saint_denis.choice.leave");
    for (std::uint64_t entropy = 0; entropy < leaveFamily.size(); ++entropy) {
        const auto* selected = leaveSelector.Choose(base, "saint_denis.choice.leave", entropy);
        assert(selected);
        chosen.insert(selected->id);
    }
    assert(chosen.size() == leaveFamily.size());
    return 0;
}
