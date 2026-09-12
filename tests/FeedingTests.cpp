#include "nightwalker/core/Config.h"
#include "nightwalker/systems/FeedingMath.h"
#include "nightwalker/systems/HiddenResource.h"
#include "nightwalker/systems/VampireFeedPresentation.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures=0;
void Check(bool value,const char* name){if(!value){++failures;std::cerr<<"FAIL: "<<name<<'\n';}}
}

int main(){
 using namespace nightwalker;
 systems::HiddenResource resource{-5.0};
 Check(resource.Value()==0.0,"resource lower clamp");
 resource.Gain(25.0);Check(resource.Value()==25.0,"resource gain");
 resource.Gain(500.0);Check(resource.Value()==100.0,"resource upper clamp");
 resource.Reset(62.5);Check(resource.Value()==62.5&&resource.Normalized()==0.625,"resource reset/normalized");

 game::Vec3 a{0.0F,0.0F,1.0F};game::Vec3 b{3.0F,4.0F,1.5F};
 Check(systems::feeding_math::DistanceSquared(a,b)==25.25,"distance squared");
 Check(systems::feeding_math::WithinRange(a,b,5.1),"range accepts nearby");
 Check(!systems::feeding_math::WithinRange(a,b,5.0),"range rejects outside");
 Check(systems::feeding_math::VerticalAligned(a,b,0.5),"vertical boundary accepted");
 Check(!systems::feeding_math::VerticalAligned(a,b,0.49),"vertical excess rejected");

 const game::Vec3 target{0.0F,0.0F,0.0F};
 const game::Vec3 facing{0.0F,1.0F,0.0F};
 Check(systems::VampireFeedPresentation::PreferRearStyle({0.0F,-1.0F,0.0F},target,facing),
       "feed presentation prefers rear style from behind");
 Check(!systems::VampireFeedPresentation::PreferRearStyle({0.0F,1.0F,0.0F},target,facing),
       "feed presentation keeps front style from front");
 Check(!systems::VampireFeedPresentation::PreferRearStyle({0.0F,-1.0F,0.0F},target,{}),
       "feed presentation falls back when heading vector unavailable");

 std::string diagnostics;
 const auto cfg=core::Config::Parse(
  "[Feeding]\nEnabled=true\nAllowNonLethal=true\nAllowAnimalFeeding=false\nHiddenBloodEnabled=true\nInitialBlood=125\nHungerRestoreSip=30\nHungerRestoreDrain=150\nHealthRestoreSip=15\nHealthRestoreDrain=999\nMaxDistance=9\nAlignMs=1\nGrabMs=500\nSipDurationMs=100\nDrainDurationMs=4300\nReleaseMs=1200\nStateTimeoutMs=500\n",
  [&](std::string_view m){diagnostics+=m;diagnostics+='\n';});
 Check(cfg.IsFeatureEnabled(core::Feature::Feeding),"feeding feature enabled");
 Check(!cfg.feeding.allowAnimalFeeding,"animal feeding opt-in default path");
 Check(cfg.feeding.initialBlood==100.0,"initial resource clamp");
 Check(cfg.feeding.sipBloodGain==30.0,"legacy sip hunger alias");
 Check(cfg.feeding.drainBloodGain==100.0,"legacy drain hunger alias clamp");
 Check(cfg.feeding.healthRestoreDrain==300,"drain health clamp");
 Check(cfg.feeding.maxDistance==2.5,"feed distance clamp");
 Check(cfg.feeding.alignMs==100,"align clamp");
 Check(cfg.feeding.sipDurationMs==300,"sip duration clamp");
 Check(cfg.feeding.releaseMs==1000,"release clamp");
 Check(cfg.feeding.stateTimeoutMs==4550,"timeout raised over longest stage");
 Check(!diagnostics.empty(),"feeding clamp diagnostics");

 if(failures){std::cerr<<failures<<" test(s) failed\n";return EXIT_FAILURE;}
 std::cout<<"Nightwalker feeding tests passed\n";return EXIT_SUCCESS;
}
