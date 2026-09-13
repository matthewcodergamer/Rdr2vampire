#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
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

    NarrativeCatalog catalog{};
    std::vector<std::string> warnings;
    assert(ParseNarrativeScript(ReadAll("content/Nightwalker.dialogue"), catalog,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());

    const auto* firstA = catalog.Find("saint_denis.pre_fight.first_contact_01");
    const auto* firstB = catalog.Find("saint_denis.pre_fight.first_contact_02");
    const auto* questionA = catalog.Find("saint_denis.choice.question.05");
    const auto* questionB = catalog.Find("saint_denis.choice.question.06");
    assert(firstA && firstA->lines.size() == 2);
    assert(firstB && firstB->lines.size() == 2);
    assert(questionA && questionA->lines.size() == 2);
    assert(questionB && questionB->lines.size() == 2);

    assert(firstA->lines[0].audioId == "nw.audio.sd.first_contact.nearer.01");
    assert(firstA->lines[1].audioId == "nw.audio.sd.first_contact.wiser.01");
    assert(firstB->lines[1].audioId == "nw.audio.sd.first_contact.wiser.02");
    assert(questionA->lines[0].audioId == "nw.audio.sd.question.names.01");
    assert(questionA->lines[1].audioId == "nw.audio.sd.question.none.01");
    assert(questionB->lines[0].audioId == "nw.audio.sd.question.names.02");
    assert(questionB->lines[1].audioId == "nw.audio.sd.question.none.02");

    NarrativeAudioManifest manifest{};
    warnings.clear();
    assert(manifest.Parse(ReadAll("content/Nightwalker.audio"),
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    assert(manifest.Count() == 7);

    const std::vector<std::string> requiredIds{
        "nw.audio.sd.first_contact.nearer.01",
        "nw.audio.sd.first_contact.wiser.01",
        "nw.audio.sd.first_contact.wiser.02",
        "nw.audio.sd.question.names.01",
        "nw.audio.sd.question.names.02",
        "nw.audio.sd.question.none.01",
        "nw.audio.sd.question.none.02",
    };
    for (const auto& id : requiredIds) {
        const auto path = manifest.Resolve(id, "NightwalkerRoot");
        assert(path.has_value());
        assert(path->extension() == ".wav");
        assert(path->filename().string() == id + ".wav");
    }

    NarrativeVariantSelector selector{};
    bool sawFirstA = false;
    bool sawFirstB = false;
    const auto preFight = catalog.FindFamily(ids::kSaintDenisPreFight);
    for (std::uint64_t entropy = 0; entropy < preFight.size(); ++entropy) {
        const auto* selected = selector.Choose(catalog, ids::kSaintDenisPreFight, entropy);
        assert(selected);
        sawFirstA = sawFirstA || selected->id == firstA->id;
        sawFirstB = sawFirstB || selected->id == firstB->id;
    }
    assert(sawFirstA && sawFirstB);
    return 0;
}
