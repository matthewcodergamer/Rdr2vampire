#include "nightwalker/narrative/NarrativePlayback.h"
#include "nightwalker/narrative/NarrativeScript.h"
#include "nightwalker/narrative/NarrativeSettingsLoader.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main(){
 using namespace nightwalker;
 using namespace nightwalker::narrative;

 NarrativeCatalog catalog{};std::vector<std::string>warnings;
 const std::string script=
  "schema=1\n"
  "line=test.pre|l1|THE VAMPIRE|text.1|audio.1|1000|First original line.\n"
  "line=test.pre|l2||text.2||1500|Second original line.\n";
 assert(ParseNarrativeScript(script,catalog,[&](std::string_view m){warnings.emplace_back(m);}));
 const auto* seq=catalog.Find("test.pre");assert(seq&&seq->lines.size()==2);
 assert(seq->lines[0].speaker=="THE VAMPIRE");assert(seq->lines[0].audioId=="audio.1");

 NarrativePlayback playback{};assert(playback.Start(*seq,100,5000));
 assert(playback.Active());assert(playback.CurrentLine()->id=="l1");
 playback.Update(1099);assert(playback.CurrentLine()->id=="l1");
 playback.Update(1100);assert(playback.CurrentLine()->id=="l2");
 playback.Skip(1200);assert(!playback.Active());
 assert(playback.Start(*seq,2000,700));playback.Update(2700);assert(!playback.Active());

 NarrativeCatalog future{};
 assert(!ParseNarrativeScript("schema=99\nline=x|y||z||1000|No.\n",future));
 NarrativeCatalog malformed{};
 assert(!ParseNarrativeScript("schema=1\nline=bad\n",malformed));

 const auto builtIn=BuiltInNarrativeCatalog();
 assert(builtIn.Find(ids::kSaintDenisPreFight));
 assert(builtIn.Find(ids::kSaintDenisPostDefeat));
 assert(builtIn.Find(ids::kSaintDenisClueBloodlessBody));
 assert(builtIn.Find(ids::kSaintDenisOutcomeSpared));

 const auto wrapped=WrapSubtitle("This is a deliberately longer subtitle sentence that must wrap without requiring any RDR2 runtime dependency.",32,3);
 assert(!wrapped.empty());assert(wrapped.size()<=3);for(const auto& line:wrapped)assert(line.size()<=32);

 const auto temp=std::filesystem::temp_directory_path()/"nightwalker_narrative_test.ini";
 {std::ofstream out(temp);out<<"[Narrative]\nEnabled=false\nMaxConfrontationHoldMs=999999\nMaxSequenceMs=1\nSkipKey=0x20\nOptionalAudio=false\n";}
 core::NarrativeSettings settings{};warnings.clear();LoadNarrativeSettings(temp,settings,[&](std::string_view m){warnings.emplace_back(m);});
 std::error_code ec;std::filesystem::remove(temp,ec);
 assert(!settings.enabled);assert(settings.maxConfrontationHoldMs==10000);assert(settings.maxSequenceMs==1000);assert(settings.skipKey==0x20);assert(!settings.optionalAudio);assert(!warnings.empty());

 return 0;
}
