#include "nightwalker/systems/VampireAIController.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {
constexpr float kRelocationVerificationTolerance = 1.0F;
constexpr float kRetreatDotThreshold = 0.35F;
float HorizontalSpeed(const game::Vec3& v) noexcept { return std::sqrt(v.x*v.x+v.y*v.y); }
float Dot2D(const game::Vec3& a,const game::Vec3& b) noexcept { return a.x*b.x+a.y*b.y; }
}

bool VampireAIController::BeginShadowstep(std::uint64_t nowMs) noexcept {
 if(!ValidCombatActors()||!plan_.valid)return false;departPosition_=api_.EntityCoords(vampire_);if(presentationSettings_.smokeFx)presentationApi_.PlayShadowSmoke(departPosition_,0.86F);
 presentationWatchdog_.RestoreAll();const game::PedHandle ownedPed=vampire_;
 if(!presentationWatchdog_.Own(core::OwnedState::Visibility,[this,ownedPed]{presentationApi_.RestorePedAppearance(ownedPed);} ))return false;
 if(!presentationApi_.SetPedVisible(vampire_,false)){presentationWatchdog_.Restore(core::OwnedState::Visibility);return false;}
 combatApi_.TaskStandStill(vampire_,static_cast<int>(presentationSettings_.disappearMs+100U));
 if(!api_.SetEntityCoordsNoOffset(vampire_,plan_.destination)){RestoreOwnedPresentation();return false;}
 const game::Vec3 actual=api_.EntityCoords(vampire_);if(shadowstep_math::Distance3D(actual,plan_.destination)>kRelocationVerificationTolerance){if(api_.PedAlive(vampire_))api_.SetEntityCoordsNoOffset(vampire_,departPosition_);RestoreOwnedPresentation();return false;}
 hiddenUntilMs_=nowMs+presentationSettings_.disappearMs;Transition(VampireAiState::HiddenTransit,nowMs);return true;
}

bool VampireAIController::UpdateArrivalCarry(std::uint64_t nowMs) noexcept {
 if(shadowstep_math::Distance2D(carryStart_,carryDestination_)<=0.05F)return true;
 const std::uint64_t elapsed=nowMs>=stateStartedMs_?nowMs-stateStartedMs_:0;const float duration=static_cast<float>(std::max<std::uint32_t>(1,presentationSettings_.carryMs));const float t=std::clamp(static_cast<float>(elapsed)/duration,0.0F,1.0F);
 const game::Vec3 desired{carryStart_.x+(carryDestination_.x-carryStart_.x)*t,carryStart_.y+(carryDestination_.y-carryStart_.y)*t,carryStart_.z+(carryDestination_.z-carryStart_.z)*t};
 const game::Vec3 current=api_.EntityCoords(vampire_);const auto safe=resolver_.ResolveToPoint(vampire_,current,desired,false);
 if(!safe.valid){logger_.Write(util::LogLevel::Debug,std::string("Vampire arrival carry stopped early: ")+ShadowstepResolver::ReasonText(safe.reason));return true;}
 if(!api_.SetEntityCoordsNoOffset(vampire_,safe.finalPosition))return true;return t>=1.0F;
}

bool VampireAIController::PlayerRetreating() const noexcept {
 if(!ValidCombatActors())return false;const game::Vec3 velocity=combatApi_.EntityVelocity(player_);if(HorizontalSpeed(velocity)<static_cast<float>(config_.vampireAi.retreatSpeedThreshold))return false;
 game::Vec3 moveDirection{};if(!shadowstep_math::NormalizeHorizontal(velocity,moveDirection))return false;const game::Vec3 vampirePos=api_.EntityCoords(vampire_),playerPos=api_.EntityCoords(player_);game::Vec3 away{playerPos.x-vampirePos.x,playerPos.y-vampirePos.y,0.0F},awayDirection{};if(!shadowstep_math::NormalizeHorizontal(away,awayDirection))return false;return Dot2D(moveDirection,awayDirection)>=kRetreatDotThreshold;
}

} // namespace nightwalker::systems
