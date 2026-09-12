#include "nightwalker/core/Config.h"
#include "ConfigInternal.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>

namespace nightwalker::core {
namespace {
std::string Trim(std::string v){auto p=[](unsigned char c){return !std::isspace(c);};v.erase(v.begin(),std::find_if(v.begin(),v.end(),p));v.erase(std::find_if(v.rbegin(),v.rend(),p).base(),v.end());return v;}
std::string Lower(std::string v){std::transform(v.begin(),v.end(),v.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return v;}
bool ParseBool(std::string_view t,bool& v){const auto s=Lower(Trim(std::string(t)));if(s=="true"||s=="1"||s=="yes"||s=="on"){v=true;return true;}if(s=="false"||s=="0"||s=="no"||s=="off"){v=false;return true;}return false;}
template<class T>bool ParseNumber(std::string_view t,T& v){const auto s=Trim(std::string(t));if(s.empty())return false;T p{};auto r=std::from_chars(s.data(),s.data()+s.size(),p);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())return false;v=p;return true;}
bool ParseHotkey(std::string_view t,int& v){const auto s=Trim(std::string(t));if(s.empty())return false;int base=10;const char* b=s.c_str();if(s.size()>2&&s[0]=='0'&&(s[1]=='x'||s[1]=='X')){base=16;b+=2;}unsigned p=0;const char* e=s.c_str()+s.size();auto r=std::from_chars(b,e,p,base);if(r.ec!=std::errc{}||r.ptr!=e||p>255)return false;v=static_cast<int>(p);return true;}
void Invalid(const Config::DiagnosticSink& d,std::size_t line,std::string_view key){if(d)d("Invalid value at line "+std::to_string(line)+" for "+std::string(key)+"; using the previous/default value.");}
}

Config Config::Parse(std::string_view text,DiagnosticSink diagnostics){
 Config c{};std::istringstream stream{std::string(text)};std::string section,line;std::size_t n=0;
 while(std::getline(stream,line)){
  ++n;line=Trim(line);if(line.empty()||line[0]==';'||line[0]=='#')continue;
  if(line.front()=='['&&line.back()==']'){section=Lower(Trim(line.substr(1,line.size()-2)));continue;}
  const auto eq=line.find('=');if(eq==std::string::npos){if(diagnostics)diagnostics("Ignoring malformed config line "+std::to_string(n)+".");continue;}
  const auto key=Lower(Trim(line.substr(0,eq)));const auto value=Trim(line.substr(eq+1));bool handled=true,parsed=true;
  if(section=="general"){
   if(key=="enabled")parsed=ParseBool(value,c.enabled);else if(key=="debugmode")parsed=ParseBool(value,c.debug.enabled);else handled=false;
  }else if(section=="debug"){
   if(key=="enabled")parsed=ParseBool(value,c.debug.enabled);else if(key=="profileruntime")parsed=ParseBool(value,c.debug.profileRuntime);else if(key=="shadowstephotkey")parsed=ParseHotkey(value,c.debug.shadowstepHotkey);else if(key=="restorehotkey")parsed=ParseHotkey(value,c.debug.restoreHotkey);else if(key=="reloadhotkey")parsed=ParseHotkey(value,c.debug.reloadHotkey);else handled=false;
  }else if(section=="shadowstep"){
   if(key=="enabled")parsed=ParseBool(value,c.shadowstep.enabled);else if(key=="quickdistance")parsed=ParseNumber(value,c.shadowstep.quickDistance);else if(key=="aimdistance")parsed=ParseNumber(value,c.shadowstep.aimDistance);else if(key=="cooldownms")parsed=ParseNumber(value,c.shadowstep.cooldownMs);else if(key=="maxverticaldelta"||key=="maxverticalrise")parsed=ParseNumber(value,c.shadowstep.maxVerticalDelta);else if(key=="validationtimeoutms")parsed=ParseNumber(value,c.shadowstep.validationTimeoutMs);else if(key=="wallclearance")parsed=ParseNumber(value,c.shadowstep.wallClearance);else handled=false;
  }else if(section=="movement"){
   if(key=="enabled")parsed=ParseBool(value,c.movement.enabled);else if(key=="sprintmoverate")parsed=ParseNumber(value,c.movement.sprintMoveRate);else if(key=="accelerationms")parsed=ParseNumber(value,c.movement.accelerationMs);else if(key=="burstdurationms"||key=="maxburstms")parsed=ParseNumber(value,c.movement.burstDurationMs);else if(key=="recoveryms")parsed=ParseNumber(value,c.movement.recoveryMs);else if(key=="dismountrecoveryms")parsed=ParseNumber(value,c.movement.dismountRecoveryMs);else if(key=="activationdistance")parsed=ParseNumber(value,c.movement.activationDistance);else if(key=="minvelocity")parsed=ParseNumber(value,c.movement.minVelocity);else if(key=="trailfx")parsed=ParseBool(value,c.movement.trailFx);else if(key=="trailintervalms")parsed=ParseNumber(value,c.movement.trailIntervalMs);else handled=false;
  }else if(section=="feeding"){
   if(key=="enabled")parsed=ParseBool(value,c.feeding.enabled);else if(key=="allownonlethal")parsed=ParseBool(value,c.feeding.allowNonLethal);else if(key=="allowanimalfeeding")parsed=ParseBool(value,c.feeding.allowAnimalFeeding);else if(key=="hiddenbloodenabled"||key=="bloodenabled")parsed=ParseBool(value,c.feeding.hiddenBloodEnabled);else if(key=="initialblood"||key=="initialhunger")parsed=ParseNumber(value,c.feeding.initialBlood);else if(key=="sipbloodgain"||key=="hungerrestoresip")parsed=ParseNumber(value,c.feeding.sipBloodGain);else if(key=="drainbloodgain"||key=="hungerrestoredrain")parsed=ParseNumber(value,c.feeding.drainBloodGain);else if(key=="healthrestoresip")parsed=ParseNumber(value,c.feeding.healthRestoreSip);else if(key=="healthrestoredrain")parsed=ParseNumber(value,c.feeding.healthRestoreDrain);else if(key=="maxdistance")parsed=ParseNumber(value,c.feeding.maxDistance);else if(key=="alignms")parsed=ParseNumber(value,c.feeding.alignMs);else if(key=="grabms")parsed=ParseNumber(value,c.feeding.grabMs);else if(key=="sipdurationms")parsed=ParseNumber(value,c.feeding.sipDurationMs);else if(key=="draindurationms")parsed=ParseNumber(value,c.feeding.drainDurationMs);else if(key=="releasems")parsed=ParseNumber(value,c.feeding.releaseMs);else if(key=="statetimeoutms")parsed=ParseNumber(value,c.feeding.stateTimeoutMs);else handled=false;
  }else if(section=="combat"){
   if(key=="enabled")parsed=ParseBool(value,c.combat.enabled);else if(key=="maxdistance")parsed=ParseNumber(value,c.combat.maxDistance);else if(key=="heavywindupms")parsed=ParseNumber(value,c.combat.heavyWindupMs);else if(key=="shadowstepfollowupwindupms")parsed=ParseNumber(value,c.combat.shadowstepFollowupWindupMs);else if(key=="strikewindowms")parsed=ParseNumber(value,c.combat.strikeWindowMs);else if(key=="recoveryms")parsed=ParseNumber(value,c.combat.recoveryMs);else if(key=="grabalignms")parsed=ParseNumber(value,c.combat.grabAlignMs);else if(key=="grabholdms")parsed=ParseNumber(value,c.combat.grabHoldMs);else if(key=="combatfeedholdms"||key=="biteholdms")parsed=ParseNumber(value,c.combat.feedHoldMs);else if(key=="throwragdollms")parsed=ParseNumber(value,c.combat.throwRagdollMs);else if(key=="statetimeoutms")parsed=ParseNumber(value,c.combat.stateTimeoutMs);else if(key=="strikebonus")parsed=ParseNumber(value,c.combat.strikeBonus);else if(key=="combatfeedcost"||key=="bitedamage")parsed=ParseNumber(value,c.combat.combatFeedCost);else if(key=="combatfeedrestore"||key=="biteheal")parsed=ParseNumber(value,c.combat.combatFeedRestore);else if(key=="combatfeedbloodgain"||key=="bitebloodgain")parsed=ParseNumber(value,c.combat.combatFeedBloodGain);else if(key=="strikemoverate")parsed=ParseNumber(value,c.combat.strikeMoveRate);else if(key=="throwhorizontalforce")parsed=ParseNumber(value,c.combat.throwHorizontalForce);else if(key=="throwupforce")parsed=ParseNumber(value,c.combat.throwUpForce);else if(key=="throwprojectionmeters")parsed=ParseNumber(value,c.combat.throwProjectionMeters);else if(key=="bossspecialcooldownms")parsed=ParseNumber(value,c.combat.bossSpecialCooldownMs);else handled=false;
  }else if(section=="encounter"||section=="encounter.saintdenis"){
   if(key=="enabled")parsed=ParseBool(value,c.encounter.enabled);else if(key=="starthour")parsed=ParseNumber(value,c.encounter.startHour);else if(key=="endhour")parsed=ParseNumber(value,c.encounter.endHour);else if(key=="respawncooldownhours")parsed=ParseNumber(value,c.encounter.respawnCooldownHours);else handled=false;
  }else if(section=="vampireai"){
   if(key=="enabled")parsed=ParseBool(value,c.vampireAi.enabled);else if(key=="shadowstepmindistance")parsed=ParseNumber(value,c.vampireAi.shadowstepMinDistance);else if(key=="shadowstepmaxdistance")parsed=ParseNumber(value,c.vampireAi.shadowstepMaxDistance);else if(key=="strikingrange")parsed=ParseNumber(value,c.vampireAi.strikingRange);else if(key=="predictionms")parsed=ParseNumber(value,c.vampireAi.predictionMs);else if(key=="decisionintervalms")parsed=ParseNumber(value,c.vampireAi.decisionIntervalMs);else if(key=="shadowstepcooldownms")parsed=ParseNumber(value,c.vampireAi.shadowstepCooldownMs);else if(key=="telegraphms")parsed=ParseNumber(value,c.vampireAi.telegraphMs);else if(key=="recoveryms")parsed=ParseNumber(value,c.vampireAi.recoveryMs);else if(key=="evadecooldownms")parsed=ParseNumber(value,c.vampireAi.evadeCooldownMs);else if(key=="retreatspeedthreshold")parsed=ParseNumber(value,c.vampireAi.retreatSpeedThreshold);else handled=false;
  }else if(section=="bosshud"){
   if(key=="enabled")parsed=ParseBool(value,c.bossHud.enabled);else if(key=="displayname"){if(value.empty())parsed=false;else c.bossHud.displayName=value;}else if(key=="idleseconds")parsed=ParseNumber(value,c.bossHud.idleSeconds);else if(key=="fadeseconds")parsed=ParseNumber(value,c.bossHud.fadeSeconds);else if(key=="deathholdseconds")parsed=ParseNumber(value,c.bossHud.deathHoldSeconds);else if(key=="shownumerichealth")parsed=ParseBool(value,c.bossHud.showNumericHealth);else handled=false;
  }else handled=false;
  if(handled&&!parsed)Invalid(diagnostics,n,key);
 }
 config_internal::Validate(c,diagnostics);return c;
}

} // namespace nightwalker::core
