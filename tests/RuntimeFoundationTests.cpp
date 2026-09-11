#include "nightwalker/core/Config.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/util/Timing.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures=0;
void Check(bool value,const char* name){if(!value){++failures;std::cerr<<"FAIL: "<<name<<'\n';}}
}

int main(){using namespace nightwalker::core;using nightwalker::util::Deadline;
 std::string diagnostics;
 auto cfg=Config::Parse("[General]\nDebugMode=true\n[Shadowstep]\nAimDistance=999\nCooldownMs=oops\n[Movement]\nSprintMoveRate=9\n[Encounter.SaintDenis]\nStartHour=-4\nEndHour=44\n[BossHUD]\nShowNumericHealth=true\nFadeSeconds=0\n",[&](std::string_view m){diagnostics+=m;diagnostics+='\n';});
 Check(cfg.debug.enabled,"legacy DebugMode alias");Check(cfg.shadowstep.aimDistance==25.0,"aim distance clamp");Check(cfg.shadowstep.cooldownMs==550,"malformed cooldown keeps default");Check(cfg.movement.sprintMoveRate==2.0,"movement clamp");Check(cfg.encounter.startHour==0&&cfg.encounter.endHour==23,"encounter hour clamp");Check(!cfg.bossHud.showNumericHealth,"design lock numeric health");Check(cfg.bossHud.fadeSeconds==0.1,"boss fade clamp");Check(!diagnostics.empty(),"invalid config diagnostics");
 Deadline d;Check(!d.IsArmed(),"deadline starts clear");d.Arm(1000,250);Check(!d.HasElapsed(1249),"deadline before due");Check(d.HasElapsed(1250),"deadline at due");d.Clear();Check(!d.IsArmed(),"deadline clear");
 SafetyWatchdog w;int restored=0;Check(w.Own(OwnedState::Visibility,[&]{++restored;}),"watchdog owns slot");Check(!w.Own(OwnedState::Visibility,[&]{++restored;}),"watchdog rejects duplicate owner");Check(w.OwnedCount()==1,"watchdog count");Check(w.RestoreAll()==0,"watchdog restore succeeds");Check(restored==1,"restore executes once");Check(w.RestoreAll()==0&&restored==1,"restore is idempotent");Check(w.Own(OwnedState::Task,[]{throw 1;}),"watchdog owns throwing callback");Check(w.RestoreAll()==1,"watchdog reports restore failure");Check(w.OwnedCount()==0,"throwing callback ownership cleared");
 if(failures){std::cerr<<failures<<" test(s) failed\n";return EXIT_FAILURE;}std::cout<<"Nightwalker Phase 1 tests passed\n";return EXIT_SUCCESS;
}
