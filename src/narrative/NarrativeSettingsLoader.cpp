#include "nightwalker/narrative/NarrativeSettingsLoader.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string>

namespace nightwalker::narrative {
namespace {
std::string Trim(std::string value){auto keep=[](unsigned char c){return !std::isspace(c);};value.erase(value.begin(),std::find_if(value.begin(),value.end(),keep));value.erase(std::find_if(value.rbegin(),value.rend(),keep).base(),value.end());return value;}
std::string Lower(std::string value){std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return value;}
bool ParseBool(std::string_view text,bool& value){auto s=Lower(Trim(std::string(text)));if(s=="true"||s=="1"||s=="yes"||s=="on"){value=true;return true;}if(s=="false"||s=="0"||s=="no"||s=="off"){value=false;return true;}return false;}
bool ParseInt(std::string_view text,int& value){auto s=Trim(std::string(text));if(s.empty())return false;int parsed=0;auto r=std::from_chars(s.data(),s.data()+s.size(),parsed);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())return false;value=parsed;return true;}
bool ParseKey(std::string_view text,int& value){auto s=Trim(std::string(text));if(s.empty())return false;int base=10;const char* begin=s.c_str();if(s.size()>2&&s[0]=='0'&&(s[1]=='x'||s[1]=='X')){base=16;begin+=2;}unsigned parsed=0;const char* end=s.c_str()+s.size();auto r=std::from_chars(begin,end,parsed,base);if(r.ec!=std::errc{}||r.ptr!=end||parsed>255)return false;value=static_cast<int>(parsed);return true;}
void Warn(const NarrativeSettingsDiagnosticSink& d,const std::string& m){if(d)d(m);}
}

void LoadNarrativeSettings(const std::filesystem::path& path,core::NarrativeSettings& settings,NarrativeSettingsDiagnosticSink diagnostics){
 std::ifstream in(path);if(!in)return;std::string section,raw;std::size_t lineNo=0;
 while(std::getline(in,raw)){
  ++lineNo;auto line=Trim(raw);if(line.empty()||line[0]=='#'||line[0]==';')continue;
  if(line.front()=='['&&line.back()==']'){section=Lower(Trim(line.substr(1,line.size()-2)));continue;}
  if(section!="narrative")continue;auto eq=line.find('=');if(eq==std::string::npos)continue;auto key=Lower(Trim(line.substr(0,eq)));auto value=Trim(line.substr(eq+1));bool ok=true;
  if(key=="enabled")ok=ParseBool(value,settings.enabled);
  else if(key=="maxconfrontationholdms")ok=ParseInt(value,settings.maxConfrontationHoldMs);
  else if(key=="conversationwindowms")ok=ParseInt(value,settings.conversationWindowMs);
  else if(key=="maxsequencems")ok=ParseInt(value,settings.maxSequenceMs);
  else if(key=="skipkey")ok=ParseKey(value,settings.skipKey);
  else if(key=="optionalaudio")ok=ParseBool(value,settings.optionalAudio);
  else continue;
  if(!ok)Warn(diagnostics,"Invalid [Narrative] value at line "+std::to_string(lineNo)+"; previous/default value retained.");
 }
 const int beforeHold=settings.maxConfrontationHoldMs;settings.maxConfrontationHoldMs=std::clamp(settings.maxConfrontationHoldMs,900,10000);if(beforeHold!=settings.maxConfrontationHoldMs)Warn(diagnostics,"Narrative.MaxConfrontationHoldMs was clamped to a safe range.");
 const int beforeConversation=settings.conversationWindowMs;settings.conversationWindowMs=std::clamp(settings.conversationWindowMs,6000,45000);if(beforeConversation!=settings.conversationWindowMs)Warn(diagnostics,"Narrative.ConversationWindowMs was clamped to a safe range.");
 const int beforeSequence=settings.maxSequenceMs;settings.maxSequenceMs=std::clamp(settings.maxSequenceMs,1000,20000);if(beforeSequence!=settings.maxSequenceMs)Warn(diagnostics,"Narrative.MaxSequenceMs was clamped to a safe range.");
 if(settings.skipKey<=0||settings.skipKey>255){settings.skipKey=0x0D;Warn(diagnostics,"Narrative.SkipKey was invalid; Enter was restored.");}
}

} // namespace nightwalker::narrative
