#include "nightwalker/narrative/ReactiveDialogueModel.h"
namespace nightwalker::narrative {
namespace { constexpr std::uint64_t kAimHoldMs=2200; constexpr std::uint64_t kShotClassifyMs=320; }
ReactiveDialogueEvent ReactiveDialogueModel::Update(const ReactiveDialogueInput& in) noexcept {
 if(in.hitBoss && shotPending_){ shotPending_=false; shooting_=in.shooting; aimed_=in.aimed; return ReactiveDialogueEvent::ShotHit; }
 if(in.shooting && !shooting_){ shooting_=true; shotPending_=true; shotDeadlineMs_=in.nowMs+kShotClassifyMs; aimed_=in.aimed; return ReactiveDialogueEvent::ShotStarted; }
 if(!in.shooting) shooting_=false;
 if(shotPending_ && in.nowMs>=shotDeadlineMs_){ shotPending_=false; return in.hitBoss?ReactiveDialogueEvent::ShotHit:ReactiveDialogueEvent::ShotMiss; }
 if(in.question) return ReactiveDialogueEvent::Question;
 if(in.challenge) return ReactiveDialogueEvent::Challenge;
 if(in.leave) return ReactiveDialogueEvent::Leave;
 if(in.aimed && !aimed_){ aimed_=true; aimHeldEmitted_=false; aimStartedMs_=in.nowMs; return ReactiveDialogueEvent::AimStarted; }
 if(in.aimed && aimed_ && !aimHeldEmitted_ && in.nowMs-aimStartedMs_>=kAimHoldMs){ aimHeldEmitted_=true; return ReactiveDialogueEvent::AimHeld; }
 if(!in.aimed && aimed_){ aimed_=false; aimHeldEmitted_=false; aimStartedMs_=0; return ReactiveDialogueEvent::AimLowered; }
 return ReactiveDialogueEvent::None;
}
void ReactiveDialogueModel::Reset() noexcept { aimed_=false; shooting_=false; aimHeldEmitted_=false; shotPending_=false; aimStartedMs_=0; shotDeadlineMs_=0; }
}
