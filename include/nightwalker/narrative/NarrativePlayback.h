#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "nightwalker/narrative/NarrativeScript.h"

namespace nightwalker::narrative {

class NarrativePlayback final {
public:
    bool Start(const NarrativeSequence& sequence, std::uint64_t nowMs,
               std::uint64_t maxSequenceMs) noexcept;
    void Update(std::uint64_t nowMs) noexcept;
    void Skip(std::uint64_t nowMs) noexcept;
    void Cancel() noexcept;

    [[nodiscard]] bool Active() const noexcept { return sequence_ != nullptr; }
    [[nodiscard]] bool IsPlaying(std::string_view sequenceId) const noexcept;
    [[nodiscard]] const NarrativeLine* CurrentLine() const noexcept;
    [[nodiscard]] std::string_view SequenceId() const noexcept;
    [[nodiscard]] std::size_t LineIndex() const noexcept { return lineIndex_; }
    [[nodiscard]] std::uint64_t RemainingMs(std::uint64_t nowMs) const noexcept;

private:
    void Advance(std::uint64_t nowMs) noexcept;

    const NarrativeSequence* sequence_{nullptr};
    std::size_t lineIndex_{0};
    std::uint64_t lineStartedMs_{0};
    std::uint64_t sequenceDeadlineMs_{0};
};

} // namespace nightwalker::narrative
