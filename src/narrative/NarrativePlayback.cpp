#include "nightwalker/narrative/NarrativePlayback.h"

#include <algorithm>

namespace nightwalker::narrative {

bool NarrativePlayback::Start(const NarrativeSequence& sequence,std::uint64_t nowMs,std::uint64_t maxSequenceMs) noexcept {
    Cancel();
    if(sequence.lines.empty()||maxSequenceMs==0)return false;
    sequence_=&sequence;
    lineIndex_=0;
    lineStartedMs_=nowMs;
    sequenceDeadlineMs_=nowMs+maxSequenceMs;
    return true;
}

void NarrativePlayback::Update(std::uint64_t nowMs) noexcept {
    if(!sequence_)return;
    if(nowMs>=sequenceDeadlineMs_){Cancel();return;}
    const auto* line=CurrentLine();
    if(!line){Cancel();return;}
    if(nowMs-lineStartedMs_>=line->durationMs)Advance(nowMs);
}

void NarrativePlayback::Skip(std::uint64_t nowMs) noexcept {
    if(sequence_)Advance(nowMs);
}

void NarrativePlayback::Cancel() noexcept {
    sequence_=nullptr;
    lineIndex_=0;
    lineStartedMs_=0;
    sequenceDeadlineMs_=0;
}

bool NarrativePlayback::IsPlaying(std::string_view sequenceId) const noexcept {
    return sequence_&&sequence_->id==sequenceId;
}

const NarrativeLine* NarrativePlayback::CurrentLine() const noexcept {
    if(!sequence_||lineIndex_>=sequence_->lines.size())return nullptr;
    return &sequence_->lines[lineIndex_];
}

std::string_view NarrativePlayback::SequenceId() const noexcept {
    return sequence_?std::string_view(sequence_->id):std::string_view{};
}

std::uint64_t NarrativePlayback::RemainingMs(std::uint64_t nowMs) const noexcept {
    if(!sequence_||nowMs>=sequenceDeadlineMs_)return 0;
    return sequenceDeadlineMs_-nowMs;
}

void NarrativePlayback::Advance(std::uint64_t nowMs) noexcept {
    if(!sequence_)return;
    ++lineIndex_;
    if(lineIndex_>=sequence_->lines.size()){Cancel();return;}
    lineStartedMs_=nowMs;
}

} // namespace nightwalker::narrative
