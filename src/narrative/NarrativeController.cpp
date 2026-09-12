#include "nightwalker/narrative/NarrativeController.h"

#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace nightwalker::narrative {

NarrativeController::NarrativeController(game::IGameBossBarApi& textApi,
                                         game::IGameNarrativeAudioApi& audioApi,
                                         util::Logger& logger,
                                         const core::Config& config) noexcept
    : textApi_(textApi), audioApi_(audioApi), logger_(logger), config_(config) {}

bool NarrativeController::Initialize(){if(catalog_.Empty())catalog_=BuiltInNarrativeCatalog();Cancel();logger_.Write(util::LogLevel::Info,"NarrativeController initialized; subtitles are cancellable and optional audio cannot block gameplay.");return true;}

void NarrativeController::LoadScript(const std::filesystem::path& path){
 Cancel();scriptPath_=path;catalog_=BuiltInNarrativeCatalog();
 try{
  std::ifstream in(path);if(!in){logger_.Write(util::LogLevel::Info,"Nightwalker.dialogue not found/readable; built-in original subtitles are active.");return;}
  std::ostringstream buffer;buffer<<in.rdbuf();NarrativeCatalog parsed{};
  if(!ParseNarrativeScript(buffer.str(),parsed,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);})){logger_.Write(util::LogLevel::Warning,"External narrative source rejected; built-in original subtitles remain active.");return;}
  catalog_=std::move(parsed);logger_.Write(util::LogLevel::Info,"Nightwalker.dialogue loaded successfully.");
 }catch(...){logger_.Write(util::LogLevel::Error,"Narrative source load failed; built-in original subtitles remain active.");}
}

bool NarrativeController::StartSequence(std::string_view sequenceId,std::uint64_t nowMs) noexcept {
 if(!config_.IsFeatureEnabled(core::Feature::Narrative))return false;
 const auto* sequence=catalog_.Find(sequenceId);if(!sequence){logger_.Write(util::LogLevel::Warning,std::string("Narrative sequence missing: ")+std::string(sequenceId));return false;}
 audioApi_.Stop();presentedLineId_.clear();
 if(!playback_.Start(*sequence,nowMs,static_cast<std::uint64_t>(config_.narrative.maxSequenceMs)))return false;
 skipWasDown_=SkipKeyDown();PresentAudioForCurrentLine();
 if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Narrative sequence started: ")+std::string(sequenceId));
 return true;
}

void NarrativeController::Update(const core::FrameContext& frame){
 if(!config_.IsFeatureEnabled(core::Feature::Narrative)){Cancel();return;}
 if(!playback_.Active()){skipWasDown_=SkipKeyDown();return;}
 const bool down=SkipKeyDown();if(down&&!skipWasDown_)RequestSkip(frame.nowMs);skipWasDown_=down;
 const std::string before=playback_.CurrentLine()?playback_.CurrentLine()->id:std::string{};playback_.Update(frame.nowMs);
 if(!playback_.Active()){audioApi_.Stop();presentedLineId_.clear();return;}
 const auto* current=playback_.CurrentLine();if(current&&current->id!=before)PresentAudioForCurrentLine();
 DrawCurrentSubtitle();
}

void NarrativeController::RequestSkip(std::uint64_t nowMs) noexcept {if(!playback_.Active())return;audioApi_.Stop();presentedLineId_.clear();playback_.Skip(nowMs);if(playback_.Active())PresentAudioForCurrentLine();}

void NarrativeController::Cancel() noexcept {if(playback_.Active()&&config_.debug.enabled)logger_.Write(util::LogLevel::Debug,"Narrative sequence cancelled.");audioApi_.Stop();playback_.Cancel();presentedLineId_.clear();skipWasDown_=false;}
void NarrativeController::Shutdown() noexcept {Cancel();catalog_={};scriptPath_.clear();}

bool NarrativeController::SkipKeyDown() const noexcept {const int key=config_.narrative.skipKey;if(key<=0||key>255)return false;return (::GetAsyncKeyState(key)&0x8000)!=0;}

void NarrativeController::PresentAudioForCurrentLine() noexcept {
 const auto* line=playback_.CurrentLine();if(!line||line->id==presentedLineId_)return;presentedLineId_=line->id;
 if(!config_.narrative.optionalAudio||line->audioId.empty())return;
 if(!audioApi_.TryPlay(line->audioId)&&config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Optional narrative audio unavailable; subtitle fallback active for ")+line->audioId);
}

void NarrativeController::DrawCurrentSubtitle() noexcept {
 const auto* line=playback_.CurrentLine();if(!line)return;auto wrapped=WrapSubtitle(line->text,68,3);if(wrapped.empty())return;
 int width=1920,height=1080;textApi_.Resolution(width,height);const float aspect=height>0?static_cast<float>(width)/static_cast<float>(height):16.0F/9.0F;const float scale=(16.0F/9.0F)/std::max(1.0F,aspect);const float boxWidth=std::clamp(0.72F*scale,0.46F,0.74F);
 const bool speaker=!line->speaker.empty();const float top=0.705F;const float lineStep=0.034F;const float contentLines=static_cast<float>(wrapped.size()+(speaker?1:0));const float boxHeight=0.026F+contentLines*lineStep;const float centerY=top+boxHeight*0.5F;
 textApi_.Rectangle(0.5F,centerY,boxWidth,boxHeight,8,8,8,118);float y=top+0.006F;
 if(speaker){textApi_.CenteredText(line->speaker.c_str(),0.5F,y,0.31F,216,208,194,240);y+=lineStep;}
 for(const auto& text:wrapped){textApi_.CenteredText(text.c_str(),0.5F,y,0.37F,235,231,220,245);y+=lineStep;}
}

} // namespace nightwalker::narrative
