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

bool SequenceBelongsToFamily(std::string_view sequenceId,std::string_view familyId) noexcept {
 if(sequenceId==familyId)return true;
 if(sequenceId.size()<=familyId.size()||sequenceId.substr(0,familyId.size())!=familyId)return false;
 return sequenceId[familyId.size()]=='.';
}

std::vector<const NarrativeSequence*> NarrativeCatalog::FindFamily(std::string_view familyId)const{
 std::vector<const NarrativeSequence*> out;
 for(const auto& sequence:sequences)if(SequenceBelongsToFamily(sequence.id,familyId))out.push_back(&sequence);
 return out;
}

const NarrativeSequence* NarrativeVariantSelector::Choose(const NarrativeCatalog& catalog,
                                                            std::string_view familyId,
                                                            std::uint64_t entropy){
 auto stateIt=std::find_if(families_.begin(),families_.end(),[&](const FamilyState& state){return state.familyId==familyId;});
 if(stateIt==families_.end()){families_.push_back({std::string(familyId),{}, {}});stateIt=std::prev(families_.end());}
 auto& state=*stateIt;
 const auto candidates=catalog.FindFamily(familyId);
 if(candidates.empty())return nullptr;
 if(state.remainingIds.empty()){
  state.remainingIds.reserve(candidates.size());
  for(const auto* sequence:candidates)state.remainingIds.push_back(sequence->id);
 }
 state.remainingIds.erase(std::remove_if(state.remainingIds.begin(),state.remainingIds.end(),[&](const std::string& id){return catalog.Find(id)==nullptr;}),state.remainingIds.end());
 if(state.remainingIds.empty())return nullptr;
 std::size_t index=static_cast<std::size_t>(entropy%state.remainingIds.size());
 if(state.remainingIds.size()>1&&!state.lastId.empty()&&state.remainingIds[index]==state.lastId){
  const auto offset=1U+static_cast<std::size_t>((entropy>>32U)%(state.remainingIds.size()-1U));
  index=(index+offset)%state.remainingIds.size();
 }
 const std::string selectedId=state.remainingIds[index];
 state.remainingIds.erase(state.remainingIds.begin()+static_cast<std::ptrdiff_t>(index));
 state.lastId=selectedId;
 return catalog.Find(selectedId);
}

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
 Add(c,{"sd_soul_01a",std::string(ids::kSaintDenisPreFight),"THE VAMPIRE","nw.sd.soul.01a","Do not ask what I am as though the world has only two answers.","nw.audio.sd.soul.01a",1700});
 Add(c,{"sd_soul_01b",std::string(ids::kSaintDenisPreFight),"THE VAMPIRE","nw.sd.soul.01b","I have stood where life ends and found no wall there.","nw.audio.sd.soul.01b",1700});
 Add(c,{"sd_soul_01c",std::string(ids::kSaintDenisPreFight),"THE VAMPIRE","nw.sd.soul.01c","Names are for things that stay on one side.","nw.audio.sd.soul.01c",1500});

 Add(c,{"sd_soul_02a","saint_denis.pre_fight.soul_02","THE VAMPIRE","nw.sd.soul.02a","Men pray for eternal life until eternity answers them.","nw.audio.sd.soul.02a",1700});
 Add(c,{"sd_soul_02b","saint_denis.pre_fight.soul_02","THE VAMPIRE","nw.sd.soul.02b","Then they call the answer cursed.","nw.audio.sd.soul.02b",1500});
 Add(c,{"sd_soul_02c","saint_denis.pre_fight.soul_02","THE VAMPIRE","nw.sd.soul.02c","I have had centuries to enjoy the joke.","nw.audio.sd.soul.02c",1600});

 Add(c,{"sd_soul_03a","saint_denis.pre_fight.soul_03","THE VAMPIRE","nw.sd.soul.03a","Death is not life's opposite. It is its oldest shadow.","nw.audio.sd.soul.03a",1800});
 Add(c,{"sd_soul_03b","saint_denis.pre_fight.soul_03","THE VAMPIRE","nw.sd.soul.03b","I have walked between them so long neither claims me.","nw.audio.sd.soul.03b",1800});
 Add(c,{"sd_soul_03c","saint_denis.pre_fight.soul_03","THE VAMPIRE","nw.sd.soul.03c","And still you step closer.","nw.audio.sd.soul.03c",1400});

 Add(c,{"sd_soul_04a","saint_denis.pre_fight.soul_04","THE VAMPIRE","nw.sd.soul.04a","The grave was once a terror to me.","nw.audio.sd.soul.04a",1500});
 Add(c,{"sd_soul_04b","saint_denis.pre_fight.soul_04","THE VAMPIRE","nw.sd.soul.04b","Then I learned it is only a door built by those afraid to look beyond it.","nw.audio.sd.soul.04b",2100});
 Add(c,{"sd_soul_04c","saint_denis.pre_fight.soul_04","THE VAMPIRE","nw.sd.soul.04c","I stopped knocking a very long time ago.","nw.audio.sd.soul.04c",1600});

 Add(c,{"sd_soul_05a","saint_denis.pre_fight.soul_05","THE VAMPIRE","nw.sd.soul.05a","You hear a heartbeat and call it life. You hear silence and call it death.","nw.audio.sd.soul.05a",2100});
 Add(c,{"sd_soul_05b","saint_denis.pre_fight.soul_05","THE VAMPIRE","nw.sd.soul.05b","Small words for a world that has never obeyed them.","nw.audio.sd.soul.05b",1800});
 Add(c,{"sd_soul_05c","saint_denis.pre_fight.soul_05","THE VAMPIRE","nw.sd.soul.05c","I learned that before this city had a name.","nw.audio.sd.soul.05c",1600});

 Add(c,{"sd_soul_06a","saint_denis.pre_fight.soul_06","THE VAMPIRE","nw.sd.soul.06a","Hunger is not beneath reason. Hunger is reason without manners.","nw.audio.sd.soul.06a",1900});
 Add(c,{"sd_soul_06b","saint_denis.pre_fight.soul_06","THE VAMPIRE","nw.sd.soul.06b","Men call theirs duty, love, ambition.","nw.audio.sd.soul.06b",1600});
 Add(c,{"sd_soul_06c","saint_denis.pre_fight.soul_06","THE VAMPIRE","nw.sd.soul.06c","I have only been more honest with mine.","nw.audio.sd.soul.06c",1600});

 Add(c,{"sd_soul_07a","saint_denis.pre_fight.soul_07","THE VAMPIRE","nw.sd.soul.07a","Every church promises another world.","nw.audio.sd.soul.07a",1500});
 Add(c,{"sd_soul_07b","saint_denis.pre_fight.soul_07","THE VAMPIRE","nw.sd.soul.07b","I merely returned from mine.","nw.audio.sd.soul.07b",1400});
 Add(c,{"sd_soul_07c","saint_denis.pre_fight.soul_07","THE VAMPIRE","nw.sd.soul.07c","You seem disappointed that it has teeth.","nw.audio.sd.soul.07c",1600});

 Add(c,{"sd_soul_08a","saint_denis.pre_fight.soul_08","THE VAMPIRE","nw.sd.soul.08a","I watched kings pile stone toward heaven because they feared the earth beneath them.","nw.audio.sd.soul.08a",2200});
 Add(c,{"sd_soul_08b","saint_denis.pre_fight.soul_08","THE VAMPIRE","nw.sd.soul.08b","Their crowns are dust. Their prayers are forgotten.","nw.audio.sd.soul.08b",1700});
 Add(c,{"sd_soul_08c","saint_denis.pre_fight.soul_08","THE VAMPIRE","nw.sd.soul.08c","I remain.","nw.audio.sd.soul.08c",1200});

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
