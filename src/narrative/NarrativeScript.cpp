#include "nightwalker/narrative/NarrativeScript.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <utility>

namespace nightwalker::narrative {
namespace {
std::string Trim(std::string value){auto keep=[](unsigned char c){return !std::isspace(c);};value.erase(value.begin(),std::find_if(value.begin(),value.end(),keep));value.erase(std::find_if(value.rbegin(),value.rend(),keep).base(),value.end());return value;}
void Warn(const NarrativeDiagnosticSink& d,const std::string& m){if(d)d(m);}
std::vector<std::string> SplitRecord(std::string_view value){std::vector<std::string> out;std::size_t start=0;for(int i=0;i<6;++i){auto pos=value.find('|',start);if(pos==std::string_view::npos)return {};out.emplace_back(Trim(std::string(value.substr(start,pos-start))));start=pos+1;}out.emplace_back(Trim(std::string(value.substr(start))));return out;}
bool ParseInt(std::string_view value,int& out){const auto s=Trim(std::string(value));if(s.empty())return false;auto r=std::from_chars(s.data(),s.data()+s.size(),out);return r.ec==std::errc{}&&r.ptr==s.data()+s.size();}
NarrativeSequence& SequenceFor(NarrativeCatalog& c,const std::string& id){for(auto& s:c.sequences)if(s.id==id)return s;c.sequences.push_back({id,{}});return c.sequences.back();}
bool HasLineId(const NarrativeCatalog& c,std::string_view id){for(const auto& s:c.sequences)for(const auto& l:s.lines)if(l.id==id)return true;return false;}
void Add(NarrativeCatalog& c,NarrativeLine line){SequenceFor(c,line.sequenceId).lines.push_back(std::move(line));}
}

const NarrativeSequence* NarrativeCatalog::Find(std::string_view id)const noexcept{for(const auto& s:sequences)if(s.id==id)return &s;return nullptr;}

bool ParseNarrativeScript(std::string_view text,NarrativeCatalog& catalog,NarrativeDiagnosticSink diagnostics){
 NarrativeCatalog parsed{};bool schemaSeen=false;std::istringstream in{std::string(text)};std::string raw;std::size_t lineNo=0;
 while(std::getline(in,raw)){
  ++lineNo;auto line=Trim(raw);if(line.empty()||line[0]=='#'||line[0]==';')continue;
  auto eq=line.find('=');if(eq==std::string::npos){Warn(diagnostics,"Ignoring malformed narrative line "+std::to_string(lineNo)+".");continue;}
  auto key=Trim(line.substr(0,eq));auto value=Trim(line.substr(eq+1));
  if(key=="schema"){
   int schema=0;if(!ParseInt(value,schema)||schema<1){Warn(diagnostics,"Narrative schema is invalid.");return false;}
   if(schema>kNarrativeSchemaVersion){Warn(diagnostics,"Narrative schema is newer than this build; using built-in subtitles.");return false;}
   parsed.schemaVersion=schema;schemaSeen=true;continue;
  }
  if(key!="line")continue;
  auto f=SplitRecord(value);if(f.size()!=7){Warn(diagnostics,"Narrative record at line "+std::to_string(lineNo)+" must contain seven pipe-separated fields.");continue;}
  int duration=2400;if(!ParseInt(f[5],duration)){Warn(diagnostics,"Narrative duration at line "+std::to_string(lineNo)+" is invalid; using 2400 ms.");duration=2400;}
  NarrativeLine entry{f[1],f[0],f[2],f[3],f[6],f[4],static_cast<std::uint32_t>(std::clamp(duration,600,7000))};
  if(entry.id.empty()||entry.sequenceId.empty()||entry.textId.empty()||entry.text.empty()){Warn(diagnostics,"Narrative record at line "+std::to_string(lineNo)+" is incomplete and was ignored.");continue;}
  if(HasLineId(parsed,entry.id)){Warn(diagnostics,"Duplicate narrative line id "+entry.id+" was ignored.");continue;}
  Add(parsed,std::move(entry));
 }
 if(!schemaSeen)Warn(diagnostics,"Narrative schema was omitted; schema 1 compatibility was assumed.");
 if(parsed.Empty()){Warn(diagnostics,"Narrative source contained no valid records.");return false;}
 catalog=std::move(parsed);return true;
}

NarrativeCatalog BuiltInNarrativeCatalog(){
 NarrativeCatalog c{};
 Add(c,{"sd_pre_01",std::string(ids::kSaintDenisPreFight),"THE VAMPIRE","nw.sd.pre.01","You followed the dead all this way. Did you think they were leading you home?","nw.audio.sd.pre.01",2500});
 Add(c,{"sd_pre_02",std::string(ids::kSaintDenisPreFight),"THE VAMPIRE","nw.sd.pre.02","You should have left Saint Denis to its hungers.","nw.audio.sd.pre.02",2200});
 Add(c,{"sd_post_01",std::string(ids::kSaintDenisPostDefeat),"","nw.sd.post.01","Inside the vampire's coat is a strip of paper bearing the same cut mark seen near the church.","",3200});
 Add(c,{"sd_clue_body_01",std::string(ids::kSaintDenisClueBloodlessBody),"","nw.sd.clue.body.01","The corpse is pale and nearly bloodless. The mud shows it was carried here.","",3000});
 Add(c,{"sd_clue_mark_01",std::string(ids::kSaintDenisClueStoneMark),"","nw.sd.clue.mark.01","A narrow sigil has been cut beneath the old writing. The edges are fresh.","",3000});
 Add(c,{"sd_spared_01",std::string(ids::kSaintDenisOutcomeSpared),"THE VAMPIRE","nw.sd.outcome.spared.01","Mercy is a debt, hunter. Pray I never come to collect.","nw.audio.sd.spared.01",2600});
 Add(c,{"sd_withdrawn_01",std::string(ids::kSaintDenisOutcomeWithdrawn),"","nw.sd.outcome.withdrawn.01","The vampire recedes into the dark, leaving the bells to answer for him.","",2600});
 return c;
}

std::vector<std::string> WrapSubtitle(std::string_view text,std::size_t maxCharacters,std::size_t maxLines){
 std::vector<std::string> out;if(maxCharacters<8||maxLines==0)return out;std::istringstream in{std::string(text)};std::string word,current;
 while(in>>word){std::size_t n=current.empty()?word.size():current.size()+1+word.size();if(n<=maxCharacters||current.empty()){if(!current.empty())current+=' ';current+=word;continue;}out.push_back(current);current=word;if(out.size()==maxLines)break;}
 if(out.size()<maxLines&&!current.empty())out.push_back(current);std::string extra;while(in>>word){if(!extra.empty())extra+=' ';extra+=word;}if(!extra.empty()&&!out.empty()){auto& last=out.back();if(last.size()+3>maxCharacters&&last.size()>3)last.resize(maxCharacters-3);last+="...";}return out;
}

} // namespace nightwalker::narrative
