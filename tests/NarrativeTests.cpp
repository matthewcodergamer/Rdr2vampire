#include "nightwalker/narrative/NarrativePlayback.h"
#include "nightwalker/narrative/NarrativeScript.h"
#include "nightwalker/narrative/NarrativeSettingsLoader.h"
#include "nightwalker/narrative/ReactiveDialogueModel.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

void AssertFamilyExhaustsBeforeRepeat(
    const nightwalker::narrative::NarrativeCatalog& catalog,
    std::string_view familyId,
    std::uint64_t entropyBase) {
    using namespace nightwalker::narrative;

    const auto family = catalog.FindFamily(familyId);
    assert(family.size() > 1);

    NarrativeVariantSelector selector{};
    std::set<std::string> seen{};
    const NarrativeSequence* previous = nullptr;

    for (std::size_t i = 0; i < family.size(); ++i) {
        const auto* selected = selector.Choose(
            catalog, familyId, entropyBase + static_cast<std::uint64_t>(i * 73U));
        assert(selected != nullptr);
        assert(seen.insert(selected->id).second);
        previous = selected;
    }

    assert(seen.size() == family.size());

    const auto* refill = selector.Choose(catalog, familyId, entropyBase + 0xBEEFULL);
    assert(refill != nullptr);
    assert(previous != nullptr);
    assert(refill->id != previous->id);
}

} // namespace

int main() {
    using namespace nightwalker;
    using namespace nightwalker::narrative;

    NarrativeCatalog catalog{};
    std::vector<std::string> warnings{};
    const std::string script =
        "schema=1\n"
        "line=test.pre|l1|THE VAMPIRE|text.1|audio.1|1000|First original line.\n"
        "line=test.pre|l2||text.2||1500|Second original line.\n"
        "line=test.pre.alt_01|l3|THE VAMPIRE|text.3|audio.3|1200|Alternate coherent line one.\n"
        "line=test.pre.alt_02|l4|THE VAMPIRE|text.4|audio.4|1200|Alternate coherent line two.\n"
        "line=saint_denis.react.aim|a1|THE VAMPIRE|aim.1|audio.aim.1|1000|Careful.\n"
        "line=saint_denis.react.aim.02|a2|THE VAMPIRE|aim.2|audio.aim.2|1000|Your hand speaks first.\n";

    assert(ParseNarrativeScript(script, catalog,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    const auto* sequence = catalog.Find("test.pre");
    assert(sequence != nullptr && sequence->lines.size() == 2);
    assert(sequence->lines[0].speaker == "THE VAMPIRE");
    assert(sequence->lines[0].audioId == "audio.1");
    assert(SequenceBelongsToFamily("test.pre", "test.pre"));
    assert(SequenceBelongsToFamily("test.pre.alt_01", "test.pre"));
    assert(!SequenceBelongsToFamily("test.prefight", "test.pre"));
    assert(catalog.FindFamily("test.pre").size() == 3);
    assert(catalog.FindFamily("saint_denis.react.aim").size() == 2);

    AssertFamilyExhaustsBeforeRepeat(catalog, "test.pre", 1);

    NarrativePlayback playback{};
    assert(playback.Start(*sequence, 100, 5000));
    playback.Update(1099);
    assert(playback.CurrentLine()->id == "l1");
    playback.Update(1100);
    assert(playback.CurrentLine()->id == "l2");
    playback.Skip(1200);
    assert(!playback.Active());

    ReactiveDialogueModel reactive{};
    ReactiveDialogueInput input{};
    input.nowMs = 1000;
    input.aimed = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::AimStarted);
    input.nowMs = 2000;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 3200;
    assert(reactive.Update(input) == ReactiveDialogueEvent::AimHeld);
    input.nowMs = 3300;
    input.aimed = false;
    assert(reactive.Update(input) == ReactiveDialogueEvent::AimLowered);

    input = {};
    input.nowMs = 4000;
    input.shooting = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::ShotStarted);
    assert(reactive.ShotPending());
    input.nowMs = 4100;
    input.shooting = false;
    input.hitBoss = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::ShotHit);
    assert(!reactive.ShotPending());

    reactive.Reset();
    input = {};
    input.nowMs = 5000;
    input.shooting = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::ShotStarted);
    input.nowMs = 5100;
    input.shooting = false;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 5320;
    assert(reactive.Update(input) == ReactiveDialogueEvent::ShotMiss);

    reactive.Reset();
    input = {};
    input.nowMs = 6000;
    input.question = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::Question);
    input = {};
    input.nowMs = 6100;
    input.challenge = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::Challenge);
    input = {};
    input.nowMs = 6200;
    input.leave = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::Leave);

    reactive.Reset();
    input = {};
    input.nowMs = 7000;
    input.weaponKind = game::PlayerWeaponKind::Unarmed;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 7100;
    input.weaponKind = game::PlayerWeaponKind::Melee;
    assert(reactive.Update(input) == ReactiveDialogueEvent::MeleeWeaponDrawn);
    input.nowMs = 7200;
    input.weaponKind = game::PlayerWeaponKind::Unarmed;
    assert(reactive.Update(input) == ReactiveDialogueEvent::WeaponPutAway);
    input.nowMs = 7300;
    input.weaponKind = game::PlayerWeaponKind::Lasso;
    assert(reactive.Update(input) == ReactiveDialogueEvent::LassoDrawn);
    input.nowMs = 7400;
    input.weaponKind = game::PlayerWeaponKind::Thrown;
    assert(reactive.Update(input) == ReactiveDialogueEvent::ThrowableDrawn);
    input.nowMs = 7500;
    input.weaponKind = game::PlayerWeaponKind::Ranged;
    assert(reactive.Update(input) == ReactiveDialogueEvent::RangedWeaponDrawn);

    reactive.Reset();
    input = {};
    input.nowMs = 8000;
    input.weaponKind = game::PlayerWeaponKind::Unarmed;
    input.meleeEngaged = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::UnarmedAttackStarted);
    input.nowMs = 8010;
    input.hitBoss = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::UnarmedHit);
    input.nowMs = 8020;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 8030;
    input.hitBoss = false;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 8040;
    input.hitBoss = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::UnarmedHit);

    reactive.Reset();
    input = {};
    input.nowMs = 9000;
    input.weaponKind = game::PlayerWeaponKind::Melee;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 9010;
    input.meleeEngaged = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::MeleeAttackStarted);
    input.nowMs = 9020;
    input.hitBoss = true;
    assert(reactive.Update(input) == ReactiveDialogueEvent::MeleeHit);

    reactive.Reset();
    input = {};
    input.nowMs = 10000;
    input.distanceToBoss = 8.0F;
    assert(reactive.Update(input) == ReactiveDialogueEvent::None);
    input.nowMs = 10100;
    input.distanceToBoss = 2.4F;
    assert(reactive.Update(input) == ReactiveDialogueEvent::CloseApproach);
    input.nowMs = 10200;
    input.distanceToBoss = 8.0F;
    assert(reactive.Update(input) == ReactiveDialogueEvent::BackedAway);

    const auto builtIn = BuiltInNarrativeCatalog();
    const auto churchOpenings = builtIn.FindFamily(ids::kSaintDenisPreFight);
    assert(churchOpenings.size() >= 8);
    AssertFamilyExhaustsBeforeRepeat(builtIn, ids::kSaintDenisPreFight, 0x1000ULL);
    assert(builtIn.Find(ids::kSaintDenisPostDefeat) != nullptr);

    const auto wrapped = WrapSubtitle(
        "This is a deliberately longer subtitle sentence that must wrap without requiring any RDR2 runtime dependency.",
        32, 3);
    assert(!wrapped.empty() && wrapped.size() <= 3);

    const auto temp = std::filesystem::temp_directory_path() / "nightwalker_narrative_test.ini";
    {
        std::ofstream out(temp);
        out << "[Narrative]\n"
               "Enabled=false\n"
               "MaxConfrontationHoldMs=999999\n"
               "ConversationWindowMs=1\n"
               "MaxSequenceMs=1\n"
               "SkipKey=0x20\n"
               "OptionalAudio=false\n";
    }

    core::NarrativeSettings settings{};
    warnings.clear();
    LoadNarrativeSettings(temp, settings,
        [&](std::string_view message) { warnings.emplace_back(message); });
    std::error_code ec;
    std::filesystem::remove(temp, ec);
    assert(!settings.enabled);
    assert(settings.maxConfrontationHoldMs == 10000);
    assert(settings.conversationWindowMs == 6000);
    assert(settings.maxSequenceMs == 1000);
    assert(settings.skipKey == 0x20);
    assert(!settings.optionalAudio);
    assert(!warnings.empty());

    return 0;
}
