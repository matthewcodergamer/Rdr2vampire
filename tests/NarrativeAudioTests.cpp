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
        "asset=nw.audio.sd.aim.04|audio/aim/hand-speaks.wav\n",
        [&](std::string_view message) { diagnostics.emplace_back(message); });
    assert(parsed);
    assert(manifest.Count() == 2);
    assert(diagnostics.empty());

    const std::filesystem::path root = std::filesystem::path("NightwalkerRoot");
    const auto mapped = manifest.Resolve("nw.audio.sd.soul.01a", root);
    assert(mapped.has_value());
    assert(mapped->generic_string().find("audio/opening_01a.wav") != std::string::npos);

    const auto fallback = manifest.Resolve("nw.audio.sd.melee.hit.03", root);
    assert(fallback.has_value());
    assert(fallback->generic_string().find("audio/nw.audio.sd.melee.hit.03.wav") != std::string::npos);

    const auto unsafeId = manifest.Resolve("../escape", root);
    assert(!unsafeId.has_value());

    diagnostics.clear();
    NarrativeAudioManifest guarded;
    assert(guarded.Parse(
        "schema=1\n"
        "asset=good.id|audio/good.wav\n"
        "asset=bad.path|../outside.wav\n"
        "asset=bad.type|audio/not-a-wave.mp3\n"
        "asset=good.id|audio/duplicate.wav\n",
        [&](std::string_view message) { diagnostics.emplace_back(message); }));
    assert(guarded.Count() == 1);
    assert(diagnostics.size() == 3);

    NarrativeAudioManifest future;
    assert(!future.Parse("schema=2\nasset=a|audio/a.wav\n"));
    assert(future.Count() == 0);

    nightwalker::game::SubtitleOnlyNarrativeAudioApi fallbackAudio;
    assert(!fallbackAudio.TryPlay("nw.audio.sd.test"));
    fallbackAudio.Stop();

    std::cout << "Nightwalker narrative audio manifest tests passed\n";
    return 0;
}
