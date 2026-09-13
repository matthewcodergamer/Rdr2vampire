#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "nightwalker/narrative/NarrativeScript.h"

namespace {
std::string ReadAll(const char* path) {
    std::ifstream input(path);
    assert(input && "required dialogue fixture is missing");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

int main() {
    using namespace nightwalker::narrative;

    std::vector<std::string> warnings;
    NarrativeCatalog primary{};
    assert(ParseNarrativeScript(ReadAll("content/Nightwalker.dialogue"), primary,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());

    NarrativeCatalog recorded{};
    assert(ParseNarrativeScript(ReadAll("tests/fixtures/Nightwalker.voice.dialogue"), recorded,
        [&](std::string_view message) { warnings.emplace_back(message); }));
    assert(warnings.empty());
    assert(recorded.sequences.size() == 4);

    const auto openings = recorded.FindFamily(ids::kSaintDenisRecordedOpening);
    const auto questions = recorded.FindFamily(ids::kSaintDenisRecordedQuestion);
    assert(openings.size() == 2);
    assert(questions.size() == 2);
    for (const auto* sequence : openings) {
        assert(sequence && sequence->lines.size() == 2);
        assert(sequence->lines.front().audioId == "nw.audio.sd.opening.warning");
        assert(sequence->lines.front().durationMs == 2250);
        assert(sequence->lines.back().durationMs == 5200);
    }
    for (const auto* sequence : questions) {
        assert(sequence && sequence->lines.size() == 2);
        assert(sequence->lines.front().text == "Men have given me many names.");
        assert(sequence->lines.back().text ==
               "None of those men lived long enough to make one matter.");
        assert(sequence->lines.back().durationMs == 4000);
    }

    for (auto& sequence : recorded.sequences) {
        assert(primary.Find(sequence.id) == nullptr);
        primary.sequences.push_back(std::move(sequence));
    }
    assert(primary.FindFamily(ids::kSaintDenisRecordedOpening).size() == 2);
    assert(primary.FindFamily(ids::kSaintDenisRecordedQuestion).size() == 2);

    NarrativeVariantSelector openingSelector{};
    const auto* openingA = openingSelector.Choose(primary, ids::kSaintDenisRecordedOpening, 1);
    const auto* openingB = openingSelector.Choose(primary, ids::kSaintDenisRecordedOpening, 2);
    assert(openingA && openingB && openingA->id != openingB->id);

    NarrativeVariantSelector questionSelector{};
    const auto* questionA = questionSelector.Choose(primary, ids::kSaintDenisRecordedQuestion, 3);
    const auto* questionB = questionSelector.Choose(primary, ids::kSaintDenisRecordedQuestion, 4);
    assert(questionA && questionB && questionA->id != questionB->id);

    assert(SequenceBelongsToFamily(openingA->id, ids::kSaintDenisRecordedOpening));
    assert(SequenceBelongsToFamily(questionA->id, ids::kSaintDenisRecordedQuestion));
    return 0;
}
