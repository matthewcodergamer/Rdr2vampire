#include "nightwalker/systems/SaintDenisDirector.h"

namespace nightwalker::systems {

void SaintDenisDirector::Update(const core::FrameContext& frame) {
    switch (state_) {
        case SaintDenisState::Dormant:
        case SaintDenisState::Eligible:
        case SaintDenisState::Omen:
        case SaintDenisState::SpawnPending:
            UpdateSetup(frame);
            return;
        case SaintDenisState::Stalking:
        case SaintDenisState::Confrontation:
        case SaintDenisState::Combat:
            UpdateActive(frame);
            return;
        case SaintDenisState::Resolution:
        case SaintDenisState::Cooldown:
        case SaintDenisState::Abort:
        case SaintDenisState::Cleanup:
            UpdateFinish(frame);
            return;
    }
}

} // namespace nightwalker::systems
