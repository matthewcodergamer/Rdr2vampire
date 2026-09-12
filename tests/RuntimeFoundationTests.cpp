#include "nightwalker/core/Config.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/ModelStreamRequest.h"
#include "nightwalker/util/Timing.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures=0;
void Check(bool value,const char* name){if(!value){++failures;std::cerr<<"FAIL: "<<name<<'\n';}}
class FakeGameApi final:public nightwalker::game::IGameApi{
public:
 bool available{true};bool loaded{false};bool requested{false};bool released{false};
 nightwalker::game::PedHandle PlayerPed()const noexcept override{return 1;}
 bool EntityExists(nightwalker::game::EntityHandle)const noexcept override{return true;}
 bool PedAlive(nightwalker::game::PedHandle)const noexcept override{return true;}
 nightwalker::game::ModelHash EntityModel(nightwalker::game::EntityHandle)const noexcept override{return 0xD95BCB7D;}
 nightwalker::game::Vec3 EntityCoords(nightwalker::game::EntityHandle)const noexcept override{return {};}
 float EntityHeading(nightwalker::game::EntityHandle)const noexcept override{return 0.0F;}
 nightwalker::game::Vec3 OffsetFromEntity(nightwalker::game::EntityHandle,float,float,float)const noexcept override{return {};}
 bool FindSafeCoordForPed(const nightwalker::game::Vec3&,nightwalker::game::Vec3&)const noexcept override{return true;}
 bool HasWaterAt(const nightwalker::game::Vec3&,float&)const noexcept override{return false;}
 bool IsPedModelAvailable(nightwalker::game::ModelHash)const noexcept override{return available;}
 void RequestModel(nightwalker::game::ModelHash)noexcept override{requested=true;}
 bool IsModelLoaded(nightwalker::game::ModelHash)const noexcept override{return loaded;}
 void ReleaseModel(nightwalker::game::ModelHash)noexcept override{released=true;}
 nightwalker::game::PedHandle CreateLocalPed(nightwalker::game::ModelHash,const nightwalker::game::Vec3&,float)noexcept override{return 2;}
 bool DeletePed(nightwalker::game::PedHandle& ped)noexcept override{ped=0;return true;}
};
}

int main(){using namespace nightwalker::core;using nightwalker::util::Deadline;
 std::string diagnostics;
 auto cfg=Config::Parse("[General]\nDebugMode=true\n[Shadowstep]\nAimDistance=999\nCooldownMs=oops\n[Movement]\nSprintMoveRate=9\n[Encounter.SaintDenis]\nStartHour=-4\nEndHour=44\n[BossHUD]\nShowNumericHealth=true\nFadeSeconds=0\n",[&](std::string_view m){diagnostics+=m;diagnostics+='\n';});
 Check(cfg.debug.enabled,"legacy DebugMode alias");Check(cfg.shadowstep.aimDistance==25.0,"aim distance clamp");Check(cfg.shadowstep.cooldownMs==550,"malformed cooldown keeps default");Check(cfg.movement.sprintMoveRate==1.20,"movement clamp");Check(cfg.encounter.startHour==0&&cfg.encounter.endHour==23,"encounter hour clamp");Check(!cfg.bossHud.showNumericHealth,"design lock numeric health");Check(cfg.bossHud.fadeSeconds==0.1,"boss fade clamp");Check(!diagnostics.empty(),"invalid config diagnostics");
 Deadline d;Check(!d.IsArmed(),"deadline starts clear");d.Arm(1000,250);Check(!d.HasElapsed(1249),"deadline before due");Check(d.HasElapsed(1250),"deadline at due");d.Clear();Check(!d.IsArmed(),"deadline clear");
 SafetyWatchdog w;int restored=0;Check(w.Own(OwnedState::Visibility,[&]{++restored;}),"watchdog owns slot");Check(!w.Own(OwnedState::Visibility,[&]{++restored;}),"watchdog rejects duplicate owner");Check(w.OwnedCount()==1,"watchdog count");Check(w.RestoreAll()==0,"watchdog restore succeeds");Check(restored==1,"restore executes once");Check(w.RestoreAll()==0&&restored==1,"restore is idempotent");Check(w.Own(OwnedState::Task,[]{throw 1;}),"watchdog owns throwing callback");Check(w.RestoreAll()==1,"watchdog reports restore failure");Check(w.OwnedCount()==0,"throwing callback ownership cleared");
 using namespace nightwalker::game;constexpr ModelHash model=0xD95BCB7D;FakeGameApi api;ModelStreamRequest request;
 api.available=false;Check(request.Begin(api,model,1000,5000)==ModelStreamStatus::InvalidModel,"invalid model rejected");Check(!api.requested,"invalid model not requested");
 api.available=true;Check(request.Begin(api,model,2000,5000)==ModelStreamStatus::Loading,"valid model begins loading");Check(api.requested,"model request issued");Check(request.Update(api,6999)==ModelStreamStatus::Loading,"model request before timeout");api.loaded=true;Check(request.Update(api,7000)==ModelStreamStatus::Loaded,"loaded model observed");request.Release(api);Check(api.released&&request.Status()==ModelStreamStatus::Idle,"loaded model released");
 api.loaded=false;api.released=false;Check(request.Begin(api,model,10000,5000)==ModelStreamStatus::Loading,"timeout request begins");Check(request.Update(api,15000)==ModelStreamStatus::TimedOut,"model timeout fires");Check(api.released,"timed out model released");
 if(failures){std::cerr<<failures<<" test(s) failed\n";return EXIT_FAILURE;}std::cout<<"Nightwalker foundation tests passed\n";return EXIT_SUCCESS;
}
