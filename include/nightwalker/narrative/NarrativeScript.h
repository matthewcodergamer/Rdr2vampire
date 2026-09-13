#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace nightwalker::narrative {

inline constexpr int kNarrativeSchemaVersion = 1;

struct NarrativeLine final {
    std::string id{};
    std::string sequenceId{};
    std::string speaker{};
    std::string textId{};
    std::string text{};
    std::string audioId{};
    std::uint32_t durationMs{2400};
};

struct NarrativeSequence final {
    std::string id{};
    std::vector<NarrativeLine> lines{};
};

struct NarrativeCatalog final {
    int schemaVersion{kNarrativeSchemaVersion};
    std::vector<NarrativeSequence> sequences{};

    [[nodiscard]] const NarrativeSequence* Find(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<const NarrativeSequence*> FindFamily(std::string_view familyId) const;
    [[nodiscard]] bool Empty() const noexcept { return sequences.empty(); }
};

[[nodiscard]] bool SequenceBelongsToFamily(std::string_view sequenceId,
                                           std::string_view familyId) noexcept;

class NarrativeVariantSelector final {
public:
    [[nodiscard]] const NarrativeSequence* Choose(const NarrativeCatalog& catalog,
                                                  std::string_view familyId,
                                                  std::uint64_t entropy);
    void Reset() noexcept { families_.clear(); }

private:
    struct FamilyState final {
        std::string familyId{};
        std::vector<std::string> remainingIds{};
        std::string lastId{};
    };

    std::vector<FamilyState> families_{};
};

using NarrativeDiagnosticSink = std::function<void(std::string_view)>;

[[nodiscard]] bool ParseNarrativeScript(std::string_view text, NarrativeCatalog& catalog,
                                        NarrativeDiagnosticSink diagnostics = {});
[[nodiscard]] NarrativeCatalog BuiltInNarrativeCatalog();
[[nodiscard]] std::vector<std::string> WrapSubtitle(std::string_view text,
                                                    std::size_t maxCharacters = 68,
                                                    std::size_t maxLines = 3);

namespace ids {
inline constexpr std::string_view kSaintDenisPreFight = "saint_denis.pre_fight";
inline constexpr std::string_view kSaintDenisPostDefeat = "saint_denis.post_defeat";
inline constexpr std::string_view kSaintDenisClueBloodlessBody = "saint_denis.clue.bloodless_body";
inline constexpr std::string_view kSaintDenisClueStoneMark = "saint_denis.clue.stone_mark";
inline constexpr std::string_view kSaintDenisOutcomeSpared = "saint_denis.outcome.spared";
inline constexpr std::string_view kSaintDenisOutcomeWithdrawn = "saint_denis.outcome.withdrawn";
} // namespace ids

} // namespace nightwalker::narrative
