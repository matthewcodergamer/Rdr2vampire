# Nightwalker — Complete AI Build Prompt Pack

This file is designed to be copied into an AI coding agent one phase at a time. It is deliberately explicit so the agent does not improvise the project into a different game.

Repository: `matthewcodergamer/Red-dead-redemption-2-vampire-`

Primary target: **Red Dead Redemption 2 Story Mode on Windows**, implemented as a native Script Hook RDR2 `.asi` plugin, with optional LML content only if a later phase truly needs it.

Read these before working:

- `docs/DESIGN_LOCKS.md`
- `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md`
- `docs/SHADOWSTEP.md`
- `docs/BOSS_HEALTH_BAR.md`

If an older document conflicts with `docs/DESIGN_LOCKS.md`, follow `DESIGN_LOCKS.md`.

---

# How to use this pack

Do **not** paste every implementation prompt into an agent at once. Start with the Master Project Letter, then give the agent exactly one numbered phase prompt. Have it finish, test and commit that phase before continuing.

Every phase has four gates:

1. the project still builds;
2. the current feature works in isolation;
3. cleanup/failure paths work;
4. the agent reports exactly what remains unverified in-game.

Never allow an agent to claim an RDR2 native, animation dictionary, particle effect, hash or Script Hook API works unless it either already exists in the project, is verified from an appropriate source, or is clearly marked as an unverified placeholder requiring in-game research.

---

# MASTER PROJECT LETTER — paste this first

```text
You are the lead gameplay/engine programmer for a Red Dead Redemption 2 Story Mode mod named NIGHTWALKER.

Repository:
https://github.com/matthewcodergamer/Red-dead-redemption-2-vampire-

Your job is to implement the project incrementally as production-quality native C++ code, not generate a mockup, webpage or design-only prototype.

CORE FANTASY
Nightwalker expands RDR2's existing Saint Denis vampire into a cinematic supernatural encounter and vampire gameplay system. The signature movement is a Dawnwalker-inspired combat grammar: the vampire disappears for a fraction of a second, instantly repositions, reappears close to the target, carries/slides forward slightly, then attacks. This is a teleport/reposition mechanic, not merely extreme running speed.

USE RDR2'S EXISTING WORLD FIRST
RDR2 already contains the existing Saint Denis vampire ped `cs_vampire` with hash `0xD95BCB7D`. Prefer referencing content from the user's legitimate RDR2 installation at runtime instead of redistributing Rockstar assets. Reuse appropriate Rockstar-authored animations, particles, sounds, props and AI behaviors when technically and legally appropriate.

COPYRIGHT BOUNDARY
The project may be inspired by the feel of modern vampire games, but do not copy, extract, ship or reconstruct proprietary Dawnwalker code, voice recordings, dialogue, music, models or ripped assets. Write original dialogue and use original/licensed audio.

STORY MODE ONLY
Do not target RDR Online. Do not add multiplayer, network hooks or online support.

CRITICAL HUD RULE
Do not add a player blood meter, hunger meter, Shadowstep cooldown bar, power icons, ability cards, skill wheel, boss phase labels, boss power names, weaknesses, floating damage numbers or other custom RPG HUD.

The only custom combat HUD is a temporary, cinematic RED BOSS HEALTH BAR for the Saint Denis vampire. It appears when the boss fight actively engages, refreshes when combat activity occurs, fades away after several seconds of inactivity, and returns on re-engagement. It never reveals abilities or phases.

PRESENTATION
Keep the visual language grounded and compatible with RDR2. Shadowstep uses compact dark smoke/shadow effects, a very short disappearance, instant relocation, then a small arrival carry. Bats are an accent, not a constant swarm. Do not turn the screen into neon fantasy VFX.

ENGINEERING RULES
- Use modular controllers instead of one giant tick function.
- Keep entity handles/state ownership explicit.
- Validate every Shadowstep landing before teleporting.
- Never teleport into solid geometry, under the world or into unsafe water.
- Any temporary visibility/collision/invincibility/movement/camera/input change must have guaranteed restoration.
- Add watchdog cleanup for player death, boss death, cutscene/mission transition, encounter abort, script unload and internal timeout.
- Avoid expensive full-world scans every frame.
- Keep debug functionality behind config/debug flags.
- Do not silently swallow errors. Log them with enough context to diagnose.
- Do not hardcode unverified native hashes or animation names and pretend they are confirmed.
- Separate game-facing code from pure math/state logic where practical so important logic can be tested without launching RDR2.
- Keep configuration backwards-compatible as the mod evolves.

WORKFLOW
Before editing:
1. Read the repository docs and current source tree.
2. Summarize current implementation state and the exact phase you are about to implement.
3. Identify uncertain RDR2-native dependencies before coding.
4. Reuse existing abstractions instead of duplicating them.

While editing:
1. Make the smallest coherent production change that completes this phase.
2. Keep code compiling.
3. Add or update config defaults, logging and docs with the implementation.
4. Add deterministic tests for pure logic where possible.
5. Do not implement later phases unless required by this phase's interface.

Before finishing:
1. Build the x64 project if the environment permits.
2. Run available automated tests.
3. Review all changed files for cleanup regressions.
4. Report build/test results honestly.
5. Provide a short in-game verification checklist for anything that cannot be tested automatically.
6. Commit with a focused message.
7. Stop. Do not begin the next phase automatically.

Read `docs/DESIGN_LOCKS.md` as authoritative if another document conflicts with it.
```

---

# PHASE 0 PROMPT — Repository audit and build skeleton

```text
Implement PHASE 0: repository audit and a clean buildable Nightwalker skeleton.

GOAL
Make the repository understandable and ready for native RDR2 mod development before any supernatural mechanic is attempted.

REQUIRED WORK
1. Inspect the repository tree, docs and current files. Do not delete useful planning docs.
2. Establish a conventional native C++ x64 project layout, for example:
   /src
     /core
     /game
     /systems
     /ui
     /util
   /include
   /tests
   /config
   /docs
3. Create or normalize the Visual Studio solution/project needed to produce `Nightwalker.asi` from a DLL-style Script Hook RDR2 plugin build.
4. Do not commit proprietary/redistribution-restricted Script Hook binaries. Document where the developer places required SDK headers/libs locally.
5. Add compile-time project/version constants.
6. Add a minimal plugin entry point that can initialize, tick/yield in the Script Hook environment, and shut down cleanly.
7. Add build configuration for Debug and Release x64.
8. Add `.gitignore` entries for Visual Studio output, SDK-local dependencies, logs and local config overrides.
9. Add/update README build instructions, Story Mode-only warning and installation layout.
10. Add no gameplay mechanics yet.

ARCHITECTURE EXPECTATION
Create clean seams for these future modules without implementing them:
- Runtime/GameContext
- Config
- Logger
- SafetyWatchdog
- ShadowstepController
- MovementController
- FeedingController
- VampireAIController
- EncounterDirector
- BossHudController
- SaveData

Do not instantiate fake implementations just to fill files. Interfaces/stubs are acceptable only where they make the build architecture clearer.

DONE WHEN
- project builds as x64 in the expected development environment;
- target output is named `Nightwalker.asi` or can be renamed to it deterministically;
- no third-party commercial game assets are committed;
- source tree is documented;
- Story Mode-only restriction is visible in README;
- plugin has a valid initialization and shutdown path;
- no supernatural gameplay is implemented prematurely.

FINAL RESPONSE
List every file created/changed, exact build result, external SDK prerequisites, and the next phase that is now unblocked. Stop afterward.
```

---

# PHASE 1 PROMPT — Runtime, config, logging and fail-safe cleanup

```text
Implement PHASE 1: Nightwalker runtime foundation.

GOAL
Create the systems that every later mechanic depends on: deterministic lifecycle, logging, config, feature flags, time helpers and fail-safe cleanup.

REQUIRED MODULES
1. Logger
   - timestamped lines;
   - levels: debug/info/warn/error;
   - writes to `Nightwalker.log`;
   - never crashes the plugin because a log file cannot open.

2. Config
   - parse `Nightwalker.ini`;
   - supply safe defaults if missing/malformed;
   - sections for Debug, Shadowstep, Movement, Feeding, Encounter and BossHUD;
   - support feature flags;
   - log invalid values and clamp unsafe ranges.

3. Runtime/GameContext
   - owns controller lifecycles;
   - one controlled per-frame update entry;
   - explicit initialize/shutdown order;
   - no random global singletons unless unavoidable for Script Hook callback integration.

4. SafetyWatchdog
   Track temporary state that later features may mutate:
   - player visibility;
   - collision;
   - movement-rate override;
   - invincibility;
   - input restrictions;
   - camera overrides;
   - attachments/tasks under Nightwalker ownership.

   Expose idempotent restoration methods. Calling restore twice must be safe.

5. Debug input layer
   - enabled only when Debug=true;
   - define safe hotkey mapping from config;
   - log key actions;
   - do not bind common destructive game controls by default.

6. Timing helpers
   - monotonic millisecond/seconds timing for cooldown/state machines;
   - avoid frame-rate-dependent timers.

FAILURE RULES
If the runtime detects invalid player state, mission/cutscene transition or shutdown, it must ask active systems to cancel and then restore owned temporary state.

TESTS
Where possible, unit-test config parsing/clamping, timers and idempotent cleanup state bookkeeping without RDR2.

DONE WHEN
- loading the plugin produces one clean startup sequence in the log;
- malformed config does not crash it;
- debug hotkey infrastructure works;
- shutdown runs cleanup;
- future systems have explicit lifecycle hooks.

Do not implement Shadowstep yet. Stop after the foundation works.
```

---

# PHASE 2 PROMPT — RDR2 native wrappers, entity safety and vampire debug spawn

```text
Implement PHASE 2: safe game-native wrappers and first controlled `cs_vampire` spawn.

GOAL
Prove Nightwalker can safely interact with RDR2 Story Mode and the existing vampire ped before building combat mechanics.

REQUIREMENTS
1. Add a thin GameApi/native wrapper layer around the subset of RDR2 natives needed now.
2. Never bury raw native calls throughout unrelated controllers.
3. Add model streaming helpers:
   - request model;
   - timeout;
   - validate model loaded;
   - release model when safe.
4. Add entity validity helpers:
   - does entity exist;
   - is ped alive;
   - coordinate/heading retrieval;
   - safe delete/cleanup only for entities Nightwalker spawned and owns.
5. Implement a DEBUG-ONLY command to spawn `cs_vampire` (`0xD95BCB7D`) near, but not intersecting, the player.
6. Spawn out of immediate collision with the player and avoid unsafe terrain.
7. Track ownership so cleanup deletes only the debug-spawned vampire, never arbitrary vanilla entities.
8. Add a debug despawn command.
9. Log model request time, spawned entity handle and cleanup result.
10. If a native/API signature is not verified, do not invent it. Isolate and mark the research gap.

NO BOSS FIGHT YET
The spawned vampire can idle. Do not build phases, boss UI or teleport attacks here.

DONE WHEN
- debug key requests the existing vampire model and spawns one valid ped;
- second invocation does not leak unlimited duplicates;
- debug despawn safely cleans it;
- script shutdown cleans it;
- invalid model/native failure logs an error instead of crashing.

Give exact in-game verification steps and stop.
```

---

# PHASE 3 PROMPT — Shadowstep V1 safe locomotion prototype

```text
Implement PHASE 3: Shadowstep V1, focusing on geometry safety before VFX.

GOAL
A debug-controlled short-range blink that moves the player from A to B safely and can be executed repeatedly without soft-locks.

CORE STATE MACHINE
Use explicit states similar to:
Idle -> ResolveIntent -> ValidateDestination -> Depart -> Relocate -> Arrive -> Recovery -> Cooldown -> Idle
with Cancel/Error paths returning through cleanup.

INPUT
Start with one debug/action binding for a forward Shadowstep. A tap should request a point forward from the player/camera using a configurable maximum range.

DESTINATION RESOLUTION
Implement a reusable resolver that:
1. derives desired direction;
2. proposes destination at configured range;
3. raycasts/checks obstruction if supported by verified RDR2 natives;
4. shortens the step when a wall blocks the full range;
5. finds/snap-checks valid ground;
6. rejects large unsafe vertical deltas;
7. rejects deep/invalid water;
8. rejects points inside obvious geometry;
9. preserves a small clearance from walls/props;
10. returns structured result: valid, final position, reason rejected/shortened.

SAFETY
Do not teleport if destination validation is inconclusive.
Do not disable collision longer than required.
Do not leave the player invisible in this phase unless a tiny experimental hide is needed and guaranteed to restore.

CONFIG
Add conservative values for range, cooldown, max vertical delta and validation timeout.

DEBUGGING
Log requested destination, resolved destination, distance, rejection reason and elapsed state duration in Debug mode.

TESTABILITY
Put vector/range/clamp/candidate-scoring math into pure functions where practical and test them.

STRESS TEST
Create a debug counter/log workflow for 100 consecutive Shadowsteps. The player must not fall through the world, remain collisionless or become permanently modified.

DONE WHEN
- forward blink is repeatable;
- blocked paths shorten/reject instead of clipping through walls;
- unsafe ground rejects;
- cleanup recovers from cancellation;
- code is ready for presentation/VFX without rewriting the safety core.

Do not add combat targeting yet. Stop after V1 safety passes.
```

---

# PHASE 4 PROMPT — Shadowstep presentation, aim mode and attack buffering

```text
Implement PHASE 4: make Shadowstep look and feel like the signature Nightwalker mechanic.

GOAL
Turn the safe teleport from Phase 3 into: disappear -> skip space -> reappear -> small forward carry -> immediate attack opportunity.

PRESENTATION TARGET
- disappearance is extremely short, roughly a fraction of a second;
- use compact dark smoke/shadow particles if verified effects are available;
- optional small bat accent, not a permanent swarm;
- arrival effect is slightly stronger than departure;
- no bright neon magic;
- subtle sound cue if a suitable in-game or original/licensed sound path is available;
- no custom power HUD.

SEQUENCE
1. validate destination completely before departure;
2. play departure FX;
3. briefly hide/fade the player for a configurable 80–130 ms starting target;
4. relocate instantly;
5. restore visibility/collision immediately at valid destination;
6. play arrival FX;
7. apply a small 1–2 meter arrival carry/glide over a very short duration, stopping early if collision would occur;
8. open a short buffered melee-input window;
9. recover to normal movement;
10. enter cooldown internally, with NO cooldown meter.

AIMED SHADOWSTEP
Add hold-to-aim behavior if input architecture supports it cleanly:
- hold begins destination preview logic;
- release commits only if valid;
- if invalid, cancel without breaking controls;
- because custom HUD is locked down, avoid a large custom landing UI. If a marker is absolutely necessary for usability, keep it debug-only unless the project owner later approves it.

ATTACK BUFFER
If attack is pressed during departure/arrival window, remember it briefly and hand control into a valid RDR2 melee action after arrival. Do not teleport and deal damage on the same simulation instant.

WATCHDOG
Add maximum durations per state. Any state exceeding its timeout restores visibility/collision/input and returns to a safe state.

DONE WHEN
- the visual timing clearly reads as disappearance/reappearance;
- post-arrival carry creates the threatening slide without clipping;
- failed VFX does not prevent teleport cleanup;
- buffered attack feels responsive;
- no player power HUD was added.

Stop before enemy/boss AI.
```

---

# PHASE 5 PROMPT — Combat targeting and vampire Shadowstep AI

```text
Implement PHASE 5: targeted Shadowstep and enemy vampire teleport combat.

GOAL
Create the predatory behavior where the vampire can vanish while the player retreats, reappear close to an intelligent intercept/flank position, slide slightly, then attack with readable timing.

PART A — TARGETED PLAYER SHADOWSTEP
When a hostile target is explicitly selected by existing RDR2 targeting/lock context, generate safe candidate landing positions relative to the target:
- front/intercept;
- left flank;
- right flank;
- behind.

Score candidates using:
- geometry validity;
- distance to desired striking range;
- target facing;
- line of approach;
- large vertical penalties;
- avoid unsafe water;
- avoid placing player inside other entities.

PART B — NPC VAMPIRE SHADOWSTEP AI
Build `VampireAIController` with explicit combat states, for example:
Observe -> Approach -> Decide -> ShadowstepDepart -> ShadowstepArrive -> Telegraph -> Attack -> Recover
plus Evade, Reposition, FeedAttempt and Abort paths for future phases.

INTERCEPT BEHAVIOR
Estimate the player's short future position using recent velocity/movement direction. Keep prediction conservative. If the player is backing away, prefer a legal intercept/flank point near where the player is moving, not directly inside the player.

FAIRNESS
- never damage on the exact teleport frame;
- after arrival, add a short readable attack startup;
- give departure/arrival audio or VFX cues;
- impose internal cooldown and chain limits;
- do not teleport continuously behind the player every second;
- if all candidates are unsafe, use ordinary movement instead.

EVADE
Allow occasional lateral/rear Shadowstep when the player commits to an attack, but rate-limit it so the vampire is not frustratingly untouchable.

DEBUG TELEMETRY
In debug log only, record chosen candidate type, candidate scores, rejection reasons and cooldown decisions.

NO POWER REVEAL UI
Do not display `Shadowstep`, ability names, cooldowns, phases or next move to the player.

DONE WHEN
- boss/debug vampire can fight for at least five minutes without teleport spam;
- retreating player can be intercepted convincingly;
- narrow alleys safely fall back when no teleport destination exists;
- all teleport temporary state cleans up on boss/player death or encounter abort.
```

---

# PHASE 6 PROMPT — Supernatural continuous speed

```text
Implement PHASE 6: controlled supernatural movement distinct from Shadowstep.

GOAL
The vampire should feel unnaturally fast when moving continuously, while Shadowstep remains the impossible instant relocation move.

REQUIREMENTS
1. Add `MovementController` with explicit ownership of any movement-rate modifier.
2. Use verified RDR2 movement-rate/native mechanisms conservatively.
3. Add a short acceleration ramp instead of instantly setting absurd speed.
4. Clamp to a configurable safe multiplier discovered through testing.
5. Add subtle presentation:
   - mild camera/FOV response if feasible;
   - wind/footstep emphasis;
   - restrained dust/dark trail;
   - no neon streaks.
6. Automatically disable/reset during:
   - mounting/dismounting;
   - swimming;
   - falling/ragdoll;
   - cutscenes;
   - mission transitions;
   - player death;
   - script unload.
7. Never leave the player's movement multiplier changed after the feature ends.
8. Internal cooldown/cost is allowed but no custom meter.

TEST LOCATIONS
Saint Denis streets, Valentine, forest, open plains, stairs/slopes and obstacle-heavy alleys.

DONE WHEN
movement is faster and dramatic but remains steerable, collision-respecting and animation-stable enough for normal play.
```

---

# PHASE 7 PROMPT — Blood hunger logic and feeding without a custom HUD

```text
Implement PHASE 7: feeding and optional hidden blood-hunger resource.

DESIGN CONSTRAINT
The player must NOT receive a custom blood/hunger meter. If the internal resource exists, communicate condition only through subtle gameplay/presentation if needed, not a permanent HUD.

GOAL
Allow robust close-range vampire feeding using RDR2-compatible positioning/animations, with cleanup more important than spectacle.

INTERNAL RESOURCE
Implement an optional normalized `blood`/`hunger` value if required by the roadmap:
- persisted in Nightwalker save data later;
- abilities may consume it;
- feeding replenishes it;
- health regeneration may depend on it;
- clamp values;
- all balancing in config;
- no custom meter.

TARGET VALIDATION
A feed target must be:
- a valid living ped;
- human for V1 unless animal feeding is separately enabled;
- close enough;
- not mission-critical when that can be determined safely;
- not in an incompatible cutscene/task state;
- reachable/alignment-safe.

FEED STATE MACHINE
Candidate -> Align -> Grab -> FeedLoop -> Release/Drain -> Cleanup
with timeout/cancel paths at every stage.

ANIMATION RESEARCH
First investigate verified RDR2 animation/task/scenario options already available. Prefer robust Rockstar-authored paired/grab/choke/struggle behavior. Do not invent animation dictionary names. If the exact vanilla vampire feeding scene cannot be reused safely, implement the strongest verified approximation and document the gap.

MODES
- non-lethal sip: smaller gain, target survives;
- lethal drain: larger gain, target dies;
- later combat feed hook on staggered targets.

BLOOD PRESENTATION
Use restrained blood effects at the neck/contact point if a verified effect exists. Avoid excessive particle spam.

CLEANUP
If either entity dies, moves too far, mission state changes, animation times out or player cancels, detach/clear only Nightwalker-owned tasks and restore both entities to sane states.

DONE WHEN
20 varied ambient NPC feed attempts do not leave floating, attached, frozen or permanently task-locked victims.
```

---

# PHASE 8 PROMPT — Expanded vampire combat kit

```text
Implement PHASE 8: expanded vampire melee without turning the mod into an arcade power bar system.

GOAL
Give the player/boss a small set of physical supernatural attacks that integrate with RDR2's animation/physics language.

IMPLEMENT IN THIS ORDER
1. Shadowstep follow-up strike.
2. Faster/heavier unarmed claw-like melee using verified animation/task combinations.
3. Grab/throat-control approximation.
4. Throw/shove that transitions the target into ragdoll with controlled impulse.
5. Combat bite on a staggered/grabbed target.
6. Optional fear behavior for nearby civilians through AI reactions.
7. Optional regeneration logic when conditions are met.

THROAT LIFT
Do not promise a perfect custom animation if RDR2 lacks one. Prototype using paired positioning, attachment only if safe, controlled animation and camera framing. If attachment is unstable, use a shorter grab/choke approximation instead. Stability wins.

THROW
- ensure target is safe to ragdoll;
- apply bounded impulse;
- never launch peds at absurd velocities;
- avoid throwing through walls;
- release all attachments/tasks before ragdoll.

FAIR BOSS RULE
The NPC vampire may use these abilities, but must have readable animation cues and cooldown logic. No one-frame unavoidable damage.

UI
Do not show ability names, cooldowns or move lists.

DONE WHEN
combat can transition among normal melee, Shadowstep, grab/feed and throw without stuck tasks or corrupted controls.
```

---

# PHASE 9 PROMPT — Full Saint Denis nighttime encounter

```text
Implement PHASE 9: turn the existing RDR2 vampire into a reliable cinematic Saint Denis nighttime boss encounter.

GOAL
A player can enter the configured Saint Denis church/cathedral district at the correct time, experience an omen/stalking sequence, fight the vampire, and leave with every spawned entity/effect cleaned up.

ENCOUNTER DIRECTOR
Create explicit states such as:
Dormant -> Eligible -> Omen -> SpawnPending -> Stalking -> Confrontation -> Combat -> Resolution -> Cooldown
plus Abort/Cleanup.

ELIGIBILITY
Require all of the following:
- Story Mode state valid;
- configured night window (start around midnight through pre-dawn; tune later);
- player inside configured district/radius;
- no incompatible mission/cutscene;
- encounter not already active;
- encounter cooldown/save rules allow it.

SPAWN
- use `cs_vampire`;
- spawn outside direct player collision and preferably outside current camera view;
- validate ground;
- never spawn multiple bosses for one encounter;
- EncounterDirector owns the boss handle.

OMEN/STALKING
Use restrained combinations of:
- distant bats;
- small smoke or shadow movement;
- a distant scream/bell/sound cue if appropriate and verified;
- corpse/blood-clue hooks only if implementation is safe;
- vampire observing/repositioning at a distance.

Do not over-script the scene into a giant cutscene. Keep it compatible with free-roam RDR2.

COMBAT
Use the existing VampireAIController. Difficulty can evolve internally as health decreases, but DO NOT expose `Phase 1/2/3` labels or ability names to the player.

ABORT CONDITIONS
- player leaves the encounter area for long enough;
- major mission/cutscene starts;
- player dies;
- boss entity becomes invalid;
- time/world transition makes encounter unsafe;
- mod unloads.

RESOLUTION
On boss death, clean temporary entities/effects and set mod-owned completion/cooldown state. Do not overwrite RDR2's save structure directly.

DONE WHEN
encounter can start, fight, abort, restart after allowed cooldown and resolve without duplicates or stranded effects.
```

---

# PHASE 10 PROMPT — Red boss health bar only

```text
Implement PHASE 10: the single approved custom combat HUD element — a temporary red boss-health bar.

READ FIRST
Read `docs/BOSS_HEALTH_BAR.md` and `docs/DESIGN_LOCKS.md`. Treat them as requirements.

GOAL
When the Saint Denis vampire is actively fighting the player, display a restrained red boss meter. When combat goes quiet, fade it away. When the player re-engages, bring it back with the correct current health.

DO NOT ADD
- player health replacement;
- blood/hunger meter;
- stamina replacement;
- Shadowstep cooldown bar;
- icons;
- ability names;
- phase text;
- weaknesses/resistances;
- numeric boss HP unless explicitly enabled by config, default false;
- floating damage numbers.

OWNERSHIP
`EncounterDirector` explicitly provides the active boss to `BossHudController`. The HUD must never scan the world and guess which ped is a boss.

BEHAVIOR
Implement states:
Hidden -> FadeIn -> Visible -> FadeOut -> Hidden
and on death:
Visible/FadeIn -> DeathHold -> FadeOut -> Hidden.

SHOW/REFRESH WHEN
- scripted confrontation starts combat; OR
- player damages boss; OR
- boss damages player; OR
- boss performs a close aggressive action; OR
- active combat engagement is confirmed.

IDLE HIDE
Use configurable `BossBarIdleSeconds`, starting around 6.0 seconds. After no relevant combat activity, smoothly fade out.

RE-ENGAGE
Any new combat activity resets the timer and returns the bar with the correct health.

VISUAL
- centered near lower screen safe area;
- thin horizontal meter;
- deep blood-red fill;
- dark translucent backing;
- subdued off-white/warm-gray title;
- default title `THE VAMPIRE`;
- no neon glow;
- respect safe-zone/aspect handling.

HEALTH
Calculate authoritative ratio from current/max boss health and clamp 0..1. Smooth display value slightly for presentation without lying about state.

DEATH
Animate to zero, hold about 1.0–1.5 s, then fade away. No victory stats overlay.

CLEANUP
Immediate safe hide on abort, boss invalidation, player death, incompatible cutscene/mission, script unload.

TESTS
Test engage, damage, disengage, fade, re-engage, boss death, player death, boss despawn and encounter abort.

DONE WHEN
this is the ONLY new persistent combat HUD element in Nightwalker.
```

---

# PHASE 11 PROMPT — Progression and persistent gameplay without HUD clutter

```text
Implement PHASE 11: persistent Nightwalker progression/config state while preserving the no-power-HUD rule.

GOAL
Support optional unlocks/tuning without adding an on-screen skill tree or radial ability UI.

DESIGN
Progression may modify internal capabilities such as:
- Shadowstep range;
- cooldown;
- target/flank logic;
- feeding efficiency;
- regeneration;
- sprint multiplier;
- throw strength within safe limits.

PRESENTATION
Do not create an in-combat skill wheel or permanent HUD. For development, expose progression through config/debug commands. If a later user-facing menu is desired, keep it out-of-combat and explicitly approved by the owner first.

SAVE DATA
Use a separate Nightwalker-owned save/config file. Never patch RDR2's proprietary save file directly.

Include:
- schema version;
- encounter completion/cooldown state;
- optional unlock flags;
- internal blood/hunger value if retained;
- settings that truly belong to save state rather than INI.

ROBUSTNESS
- atomic-ish save strategy where practical (temp + replace);
- defaults when missing;
- migration hook for future schema versions;
- corrupt file should warn and recover instead of crashing.

DONE WHEN
state persists across restarts and no custom progression HUD was introduced.
```

---

# PHASE 12 PROMPT — Original narrative, dialogue and audio hooks

```text
Implement PHASE 12: original narrative scaffolding around the Saint Denis vampire.

COPYRIGHT RULE
Do not import Dawnwalker dialogue, voice files, music or scripts. Do not imitate a living actor's voice without permission. Use original text and original/licensed performance.

GOAL
Add a lightweight RDR2-compatible narrative layer that can support:
- strange killings near the church district;
- clues related to the existing Saint Denis vampire myth;
- confrontation dialogue;
- optional alternate encounter outcomes;
- original subtitles.

ENGINE WORK
1. Create a data-driven dialogue/subtitle format owned by the mod.
2. Add event hooks from EncounterDirector to narrative sequences.
3. Separate text/audio asset IDs from gameplay logic.
4. Gracefully fall back to subtitles if optional custom audio is missing.
5. Never block combat cleanup because a dialogue asset fails.
6. Keep cinematics short and cancellable in free roam.

WRITING DIRECTION
Original, period-appropriate, restrained, threatening. The vampire should sound old, intelligent and predatory without copying specific lines from another game.

DONE WHEN
an encounter can play original pre-fight/post-fight text with robust skip/abort behavior and no copyrighted imported audio.
```

---

# PHASE 13 PROMPT — Compatibility, performance and recovery hardening

```text
Implement PHASE 13: harden Nightwalker for long Story Mode sessions and mod conflicts.

GOAL
Remove assumptions that only work in a clean developer test and ensure the mod fails safely.

PERFORMANCE
- profile/update frequencies;
- no full ped/world scans every frame;
- cache/reuse encounter references;
- rate-limit raycasts/ground checks when not actively Shadowstepping;
- no permanent bat swarms;
- no particle leak loops;
- no repeated model requests once cached/available;
- avoid per-frame heap churn where practical.

RECOVERY MATRIX
Force cleanup for:
- player death;
- boss death;
- save/load transitions;
- cutscene start;
- mission transitions;
- fast travel;
- script reload/unload;
- invalid entity handles;
- model/VFX timeout;
- interrupted feed;
- interrupted Shadowstep;
- interrupted throw/grab;
- boss despawn;
- config feature disable while active.

COMPATIBILITY
- do not overwrite vanilla files if code-only behavior can avoid it;
- use optional LML content only where necessary;
- namespace files/config/save data clearly;
- avoid global changes to AI/player attributes that are not restored;
- document known compatibility risks.

HUD REGRESSION
Verify only the temporary red boss health bar exists as custom combat UI.

DONE WHEN
Nightwalker can survive extended free-roam test sessions and repeated encounter starts/aborts without accumulating bad state.
```

---

# PHASE 14 PROMPT — Full regression, packaging and 1.0 release

```text
Implement PHASE 14: prepare Nightwalker 1.0 release candidate.

DO NOT ADD NEW FEATURES
This phase is testing, bug fixing, documentation and packaging.

TEST MATRIX
Test at minimum:
- Arthur;
- John;
- Saint Denis dense streets;
- church/cathedral encounter zone;
- alleys;
- forest;
- open plains;
- steep slopes/stairs;
- water edges;
- mounted/unmounted transitions;
- wanted level active;
- nearby Story Mode mission state;
- player death during each major mechanic;
- boss death during special movement;
- low and high frame-rate conditions where possible;
- keyboard/mouse;
- controller.

CORE REGRESSIONS
1. Shadowstep never leaves player invisible.
2. Collision always restores.
3. Movement multiplier always resets.
4. Feed target always releases/detaches.
5. Throw/grab cannot leave target attached.
6. Boss cannot duplicate.
7. Encounter abort cleans VFX/entities.
8. Boss health bar fades/reappears correctly and never becomes permanent.
9. No boss abilities/phases are exposed by UI.
10. Save/config corruption fails gracefully.
11. Mod does not target RDR Online.
12. No proprietary Dawnwalker assets are included.
13. Release ZIP contains no developer SDK paths, logs or build junk.

RELEASE PACKAGE
Prepare a clean structure such as:
Nightwalker/
  Nightwalker.asi
  Nightwalker.ini
  README.txt or README.md
  CHANGELOG.md
  LICENSE/THIRD_PARTY notices as applicable

Optional LML content must be separate and documented.

README MUST INCLUDE
- Story Mode only;
- dependencies;
- installation;
- uninstall;
- default controls;
- configuration;
- troubleshooting/log location;
- compatibility notes;
- copyright/original-asset statement;
- known limitations.

VERSION
Set 1.0 only after the regression matrix is complete enough to support it. Otherwise label the build RC/beta honestly.

FINAL RESPONSE
Provide:
- exact build artifact name;
- tests passed/failed;
- unresolved issues;
- packaging contents;
- release checklist;
- commit/release notes.
```

---

# SPECIAL PROMPT A — Native/animation research task

Use this whenever implementation is blocked by uncertainty about RDR2 internals.

```text
Perform a focused Nightwalker RDR2 implementation research pass for the following blocked capability:

[DESCRIBE CAPABILITY]

Do not edit gameplay code yet.

Find and document the smallest verified set of RDR2/Script Hook RDR2 APIs, natives, animation dictionaries, scenarios, particle effects, model hashes or task primitives required.

Rules:
- prefer authoritative native databases, Script Hook SDK examples and well-established RDR2 modding references;
- distinguish confirmed facts from community guesses;
- do not invent hashes or dictionary names;
- record source/reference and expected function signature/use;
- identify cleanup requirements;
- identify whether the technique is Story Mode-safe;
- propose at least one fallback implementation if the ideal method is unreliable.

Write findings to an appropriate `docs/research/` markdown file, then stop so implementation can be reviewed separately.
```

---

# SPECIAL PROMPT B — Bug-fix prompt

```text
Fix this Nightwalker bug without expanding scope:

BUG:
[PASTE BUG / LOG / VIDEO OBSERVATION]

REPRODUCTION:
[STEPS]

EXPECTED:
[EXPECTED BEHAVIOR]

RULES
1. Reproduce from code/log evidence before guessing.
2. Trace ownership/state transitions involved.
3. Identify root cause, not only visible symptom.
4. Patch the smallest correct layer.
5. Preserve `docs/DESIGN_LOCKS.md` rules.
6. Add a regression test/log assertion where practical.
7. Re-run relevant build/tests.
8. Check cleanup paths specifically.
9. Do not refactor unrelated code.
10. Report root cause, changed files, verification and any remaining in-game-only test.
```

---

# SPECIAL PROMPT C — Shadowstep feel-tuning pass

```text
Tune Nightwalker's Shadowstep for feel without changing its safety architecture.

TARGET FEEL
Disappear for a split moment, skip space instantly, reappear near the intended target/position, then carry forward slightly before control/attack fully resumes.

Adjust only configuration/timing/presentation and clearly isolated feel parameters unless a bug requires deeper code.

Evaluate:
- departure-to-hide latency;
- hidden duration;
- arrival FX timing;
- arrival carry distance and duration;
- attack-buffer window;
- camera response;
- sound cue timing;
- AI telegraph delay;
- chain cooldown;
- flank/intercept selection bias.

Do not:
- increase range until geometry validation becomes unreliable;
- add giant VFX;
- add a cooldown HUD;
- make boss attacks land on the teleport frame;
- expose move names/phases.

Return a before/after tuning table and the in-game scenes used for evaluation.
```

---

# SPECIAL PROMPT D — PR/code review prompt

```text
Review the latest Nightwalker pull request as a senior C++ gameplay/mod engineer.

Prioritize defects over style.

Check specifically for:
- invalid/unverified RDR2 native assumptions;
- entity ownership mistakes;
- leaked model/particle/entity resources;
- player visibility/collision not restored;
- movement modifiers not restored;
- state-machine transitions with no timeout;
- teleport destination validation gaps;
- feed/grab attachment cleanup failures;
- boss duplication;
- expensive per-frame scans;
- frame-rate-dependent timers;
- config values not clamped;
- save corruption risks;
- online/RDR Online assumptions;
- accidental copyrighted asset inclusion;
- violations of the boss-HUD-only rule.

For every finding provide severity, file/function, why it matters in-game, and the smallest fix. If no blocking issue exists, state that clearly rather than inventing criticism.
```

---

# SPECIAL PROMPT E — AI handoff/status prompt

Use this when a different AI agent is taking over the repository.

```text
Audit the Nightwalker repository and prepare a concise implementation handoff.

Report:
1. current branch/commit;
2. latest completed phase;
3. systems that compile and exist;
4. systems that are design-only;
5. open GitHub issues relevant to the next phase;
6. known in-game blockers;
7. unverified natives/assets/animation research gaps;
8. cleanup/safety risks still open;
9. exact next smallest implementation task;
10. files the next agent must read first.

Do not modify code during this handoff. Do not assume a roadmap checkbox means a feature is actually implemented; verify from source.
```

---

# Definition of complete

Nightwalker is not complete because it has many files or because an AI says `done`.

A 1.0-quality implementation means:

- the ASI reliably loads in supported Story Mode;
- `cs_vampire` is used safely as the encounter anchor;
- Shadowstep is safe, fast, readable and visually convincing;
- the vampire can flank/intercept without unfair teleport spam;
- continuous supernatural speed always restores normal movement;
- feeding is stable across varied ambient NPCs;
- expanded melee/grab/throw paths recover cleanly;
- the Saint Denis encounter starts, aborts and resolves without duplicates;
- the **only custom combat HUD is the temporary red boss-health bar**;
- the boss bar fades after inactivity and returns on re-engagement;
- boss powers/phases remain mysterious and are never exposed by UI;
- separate Nightwalker persistence does not corrupt RDR2 saves;
- no RDR Online support is present;
- no proprietary Dawnwalker assets/code/audio are shipped;
- release packaging is clean and documented;
- long-session regression testing does not reveal accumulating broken state.

That is the finish line the AI should work toward, one verified phase at a time.
