#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "nightwalker/game/GameNarrativeAudioApi.h"
#include "nightwalker/narrative/NarrativeAudioManifest.h"

int main() {
    using nightwalker::narrative::NarrativeAudioManifest;

    std::vector<std::string> diagnostics;
    NarrativeAudioManifest manifest;
    const bool parsed = manifest.Parse(
        "schema=1\n"
        "asset=nw.audio.sd.soul.01a|audio/opening_01a.wav\n"
        "asset=nw.audio.sd.aim.04|audio/aim/hand-speaks.mp3\n",
        [&](std::string_view message) { diagnostics.emplace_back(message); });
    assert(parsed);
    assert(manifest.Count() == 2);
    assert(diagnostics.empty());

    const std::filesystem::path root = std::filesystem::path("NightwalkerRoot");
    const auto mappedWav = manifest.Resolve("nw.audio.sd.soul.01a", root);
    assert(mappedWav.has_value());
    assert(mappedWav->generic_string().find("audio/opening_01a.wav") != std::string::npos);

    const auto mappedMp3 = manifest.Resolve("nw.audio.sd.aim.04", root);
    assert(mappedMp3.has_value());
    assert(mappedMp3->generic_string().find("audio/aim/hand-speaks.mp3") != std::string::npos);

    const auto fallback = manifest.Resolve("nw.audio.sd.melee.hit.03", root);
    assert(fallback.has_value());
    assert(fallback->generic_string().find("audio/nw.audio.sd.melee.hit.03.wav") != std::string::npos);

    const auto playerFallback = manifest.Resolve("nw.audio.player.arthur.talk.01", root);
    assert(playerFallback.has_value());
    assert(playerFallback->generic_string().find(
        "audio/player/nw.audio.player.arthur.talk.01.wav") != std::string::npos);

    const auto unsafeId = manifest.Resolve("../escape", root);
    assert(!unsafeId.has_value());

    diagnostics.clear();
    NarrativeAudioManifest guarded;
    assert(guarded.Parse(
        "schema=1\n"
        "asset=good.wav|audio/good.wav\n"
        "asset=good.mp3|audio/good.mp3\n"
        "asset=bad.path|../outside.wav\n"
        "asset=bad.type|audio/not-audio.txt\n"
        "asset=good.wav|audio/duplicate.wav\n",
        [&](std::string_view message) { diagnostics.emplace_back(message); }));
    assert(guarded.Count() == 2);
    assert(diagnostics.size() == 3);

    NarrativeAudioManifest future;
    assert(!future.Parse("schema=2\nasset=a|audio/a.wav\n"));
    assert(future.Count() == 0);

    diagnostics.clear();
    NarrativeAudioManifest missingSchema;
    assert(!missingSchema.Parse(
        "asset=nw.audio.sd.soul.01a|audio/opening_01a.wav\n",
        [&](std::string_view message) { diagnostics.emplace_back(message); }));
    assert(missingSchema.Count() == 0);
    assert(diagnostics.size() == 1);
    assert(diagnostics.front().find("schema=1") != std::string::npos);

    nightwalker::game::SubtitleOnlyNarrativeAudioApi fallbackAudio;
    assert(!fallbackAudio.TryPlay("nw.audio.sd.test"));
    fallbackAudio.Stop();

    std::cout << "Nightwalker narrative audio manifest tests passed\n";
    return 0;
}
