# Nightwalker — Zero-to-Release Roadmap

## Goal

Build a polished **single-player RDR2 vampire overhaul** that turns the Saint Denis vampire idea into a complete gameplay fantasy: predatory movement, Shadowstep, supernatural speed, feeding, blood hunger, vampire combat, scripted night encounters, progression and an original story layer.

The project should feel inspired by modern vampire action RPGs without copying or redistributing another game's code, dialogue, voice files, music or proprietary assets.

---

## Feasibility snapshot

| System | Feasibility | Notes |
| --- | --- | --- |
| Existing RDR2 vampire ped | Very high | RDR2 exposes `cs_vampire` (`0xD95BCB7D`). |
| Night-only scripted encounter | Very high | Time, location, spawn and AI state are scriptable. |
| Short-range Shadowstep/blink | High | Teleport + raycast + visibility/VFX + landing validation. |
| Smoke / dust / supernatural VFX | High | Reuse RDR2 particle systems at runtime. |
| Bat burst around teleport | High | Use lightweight VFX first; optional short-lived `A_C_Bat_01` peds later. |
| Supernatural sprint | High | Movement-rate overrides plus animation/FX tuning. |
| Blood hunger / healing | Very high | Pure mod state + health/stamina natives. |
| Feeding on NPCs | Medium-high | Needs reliable synced positioning and animation discovery. |
| Shadowstep enemy AI | High | Scripted flank/behind-player destination selection. |
| Custom vampire boss encounter | High | Ped AI + health phases + ability cooldown state machine. |
| Skill wheel / custom HUD | Medium-high | Start with prompts/simple HUD, then custom overlay if needed. |
| True arbitrary wall-walking | Low-medium | RDR2 was not authored for it; fake ledge/vertical blink is safer. |
| Original quests/dialogue | High | Script logic is straightforward; content production is the work. |
| Port Dawnwalker voice/dialogue assets | Do not ship | Use original writing/performance instead of extracted copyrighted assets. |

---

# Phase 0 — Foundation and research

**Purpose:** make the repository buildable and define exactly what the mod is before touching complex mechanics.

### Tasks

- [x] Create project repository and vision.
- [x] Confirm RDR2 contains `cs_vampire`.
- [x] Define Shadowstep feel and safety rules.
- [ ] Add Script Hook RDR2 SDK locally as a developer dependency; do not commit third-party binaries unless redistribution is permitted.
- [ ] Create Visual Studio x64 `.asi` project.
- [ ] Add `Nightwalker.ini` config loader.
- [ ] Add structured logger: `Nightwalker.log`.
- [ ] Add feature flags so each system can be enabled independently.
- [ ] Add a debug menu / hotkeys for teleport, spawn, feed and effects.

### Exit criteria

Launching Story Mode loads `Nightwalker.asi`, writes one clean startup line to the log, and can be disabled without changing game files other than removing the plugin.

---

# Phase 1 — Core runtime architecture

Build the mod as several isolated systems instead of one giant tick loop.

### Runtime modules

- `GameClock` — day/night and time windows.
- `PlayerVampireController` — form, hunger, health and abilities.
- `ShadowstepController` — aiming, landing validation and teleport sequence.
- `MovementController` — sprint/burst speed and movement cleanup.
- `FeedingController` — target validation and feed state machine.
- `VfxController` — particles, optional bats, screen/camera effects.
- `VampireAIController` — enemy blink, melee, flee/reposition and boss states.
- `EncounterDirector` — location/time-based spawns and despawns.
- `Progression` — unlocks, cooldown modifiers and saved state.
- `Config` / `SaveData` / `Logger`.

### Engineering rules

- Never teleport directly from button input without first validating a landing point.
- Every temporary state change (collision, visibility, movement rate, invincibility) must have a guaranteed restore path.
- No ability is allowed to leave the player stuck if an animation or effect fails.
- Keep expensive world scans out of every-frame code; cache candidates and update them at lower frequency.
- Story Mode only. Do not design this for RDR Online.

### Exit criteria

Hotkeys can toggle vampire state, spawn the RDR2 vampire, and run a no-VFX debug blink repeatedly without crashes.

---

# Phase 2 — Shadowstep V1: the signature mechanic

This is the first feature that must feel excellent before the rest of the mod expands.

### Player controls

**Tap ability:** quick forward Shadowstep.

**Hold ability:** enter aim mode, raycast toward the camera reticle, show a valid/invalid destination marker, then release to blink.

**Combat lock-on:** if a valid hostile target is selected, allow a flank/behind-target destination instead of only raw camera direction.

### Core sequence

1. Read input and intended direction.
2. Calculate desired endpoint.
3. Raycast/capsule-check path and destination.
4. Ground-snap or reject unsafe landing.
5. Play departure VFX/audio.
6. Fade/hide the ped for only the blink window.
7. Reposition instantly.
8. Play arrival VFX/audio.
9. Restore collision/visibility.
10. Apply a tiny forward carry/slide so the move has momentum.
11. Open a short buffered attack window.
12. Apply stamina/hunger cost and cooldown.

### Landing safety

Reject or shorten the step when:

- the destination is inside solid geometry;
- the player would land below the world;
- vertical difference exceeds the current ability tier;
- the landing point is in invalid/deep water;
- a closed interior/door volume makes the destination unsafe;
- the target moves too far while the blink is being resolved.

### Feel targets

- The player should visually disappear for roughly a fraction of a second, not stand invisible for a noticeable pause.
- Arrival should be close enough to immediately attack when combat-targeting is used.
- A small 1–2 m post-arrival carry can create the "reappear then slide" look.
- Chained steps should be fast but limited by stamina/cooldown so normal travel is not completely replaced.

### Exit criteria

100 consecutive test Shadowsteps in Saint Denis streets, forest terrain, stairs and combat without falling through the map, getting stuck invisible, or embedding inside props.

---

# Phase 3 — Shadowstep V2: Dawnwalker-style combat behavior

The objective is not to copy another game's code. We recreate the *combat grammar*: vanish, instant reposition, threatening arrival.

### Combat behaviors

- **Gap close:** blink to 1.5–2.0 m in front/side of target.
- **Backstep punish:** if the target is retreating, predict a short future position and arrive at an intercept angle.
- **Flank:** select left/right destination based on free space.
- **Backstab setup:** arrive behind the target when line-of-sight and clearance allow it.
- **Evade:** blink perpendicular to incoming melee/gunfire direction.
- **Execution follow-up:** buffer melee input during arrival and immediately transition to an attack animation.

### Enemy Shadowstep AI

For a vampire NPC:

1. Player distance enters ability band.
2. AI chooses front/flank/behind based on player movement and nearby geometry.
3. Validate point.
4. Departure smoke/bats.
5. Hide/reposition.
6. Arrival slide.
7. Attack, grab or disengage.
8. Cooldown prevents spam.

This specifically supports the cinematic moment where the player is backing away, the vampire vanishes for milliseconds, then reappears close enough to strike.

### Exit criteria

Fight the vampire for five minutes in multiple Saint Denis spaces without teleport spam, geometry clipping, impossible-to-read attacks or repeated unfair one-frame hits.

---

# Phase 4 — Supernatural movement

Shadowstep handles instant relocation; supernatural speed handles continuous movement.

### Features

- Vampire sprint multiplier while the ability is active.
- Short burst acceleration rather than permanently extreme speed.
- FOV and camera shake scaled subtly with speed.
- Footstep/wind audio layer.
- Dark trailing particles or dust, not a giant fantasy aura.
- Optional stamina or blood drain while sprinting.
- Automatic reset when mounting a horse, entering cutscenes, swimming, falling or being ragdolled.

Use RDR2's movement-rate natives conservatively. The goal is fast, controlled motion that still works with RDR2 collision and animation, not a 60 m/s physics exploit.

### Exit criteria

Sprint through Saint Denis, Valentine, forest and uneven terrain without losing control, desynchronizing animations or launching from small obstacles.

---

# Phase 5 — Feeding and blood hunger

### Hunger model

Suggested normalized value: `0–100`.

- 100 = fully fed.
- Vampire abilities consume small amounts.
- Time at night slowly lowers hunger.
- Low hunger increases screen/audio feedback and eventually introduces penalties.
- Feeding restores hunger and health.

### Feed target rules

Eligible targets must be:

- alive;
- human unless animal feeding is enabled;
- not in a mission/cutscene state;
- close enough and reachable;
- not currently in an incompatible animation state.

### Feed interaction V1

Start with robust existing RDR2 grab/choke/struggle animations and synced positioning. The vampire pulls the victim in, camera moves closer, blood effect appears near the neck, then the victim is released, knocked out or killed depending on the chosen feed strength.

### Feed choices

- **Sip:** low blood gain, victim survives.
- **Drain:** large blood gain, victim dies.
- **Combat bite:** short execution-like feed on staggered enemies.
- **Animal feed:** optional morality-friendly path.

### Exit criteria

Feed on at least 20 different ambient NPC archetypes without broken positioning, floating victims, permanent AI lockups or mission NPC corruption.

---

# Phase 6 — Vampire combat kit

Build only after Shadowstep and feeding are solid.

### Abilities

- Claw/slash melee stance using selected RDR2 animations and weaponless damage.
- Shadowstep attack.
- Grab / throat lift approximation using paired animation or scripted attachment if stable.
- Throw / shove with ragdoll impulse.
- Fear pulse: nearby civilians flee; weak enemies hesitate.
- Predator sense: temporary highlight/focus mode for living targets.
- Regeneration when well-fed and out of direct damage.

### Important limitation

RDR2 does not have a full vampire animation set for every fantasy move. Prefer convincing combinations of Rockstar-authored animations, camera work, positioning, VFX and physics before attempting custom animation imports.

---

# Phase 7 — Saint Denis vampire encounter

RDR2 already gives us the perfect base model and lore hook.

### Existing-game anchor

Use `cs_vampire` rather than redistributing the model. The vanilla Easter egg already establishes a Saint Denis vampire and a midnight encounter, so the mod can feel like an expansion of something already present in the world.

### Mod encounter concept

**Location:** Saint Denis cathedral/church district, cemetery, crypt-like area or nearby alley network.

**Default active window:** midnight to pre-dawn, configurable.

**Flow:**

1. At night the encounter director checks whether the player is near the district.
2. A low-probability omen begins: bats, distant scream, church bell, corpse or blood trail.
3. Vampire spawns out of direct camera view.
4. Initial stalking state keeps distance and uses rooftops/alleys where possible.
5. If confronted, custom dialogue begins.
6. Fight enters boss state.
7. Boss gains Shadowstep at phase 1, faster chained steps at phase 2, desperation feeding at phase 3.
8. Reward grants a vampire progression unlock or unique original item/config flag.
9. Encounter enters a cooldown and persists in save data.

### Exit criteria

Encounter starts and ends cleanly without duplicating the vampire, breaking law AI, persisting forever, or colliding with vanilla mission logic.

---

# Phase 8 — Progression and skill tree

Suggested branches:

### Predator
- Shadowstep range
- reduced cooldown
- flank targeting
- silent arrival
- chained step

### Blood
- larger hunger capacity
- stronger healing
- non-lethal feeding efficiency
- combat feed
- regeneration

### Beast
- sprint speed
- claw damage
- ragdoll throw
- fear radius
- damage resistance at night

Use a simple unlock menu first. Fancy presentation comes later.

---

# Phase 9 — UI and presentation

### HUD

- Blood/hunger meter.
- Shadowstep cooldown feedback.
- Valid landing marker while aiming.
- Context prompt: Feed / Drain.
- Minimal vampire state indicator.

### Ability selection

V1: keyboard/controller mappings from config.

V2: radial ability menu with slowed time while selecting.

### Accessibility/config

- Toggle camera shake.
- Toggle gore/blood intensity.
- Shadowstep range slider within safe limits.
- Cooldown multiplier.
- Hunger drain multiplier.
- Night-only powers on/off.
- Bat effects on/off for performance.

---

# Phase 10 — Original narrative layer

Do not transplant Dawnwalker dialogue or voice files. Write an original Saint Denis vampire story that belongs in the RDR2 world.

Possible structure:

- Strange killings near the cathedral.
- Clues tied to the existing vampire writings.
- A second vampire sect or survivor.
- Player can hunt the vampire, become allied with him, or accept a curse/blessing.
- Feeding choices influence how NPCs and the city react.

### Voice pipeline

1. Original script.
2. Human performance or properly licensed synthetic voice.
3. Process for period-appropriate room tone/reverb.
4. Subtitle file with every spoken line.
5. Never require copyrighted voice/audio extracted from another game.

---

# Phase 11 — Save system and world persistence

Persist only mod-owned state:

- abilities unlocked;
- hunger configuration;
- boss defeated/available;
- encounter cooldowns;
- player vampire enabled;
- progression points;
- optional morality/feeding stats.

Never overwrite RDR2 save files directly. Use a separate Nightwalker save/config file keyed to the current player profile where practical.

---

# Phase 12 — Compatibility and performance

### Compatibility targets

- Current supported Story Mode build + Script Hook RDR2 runtime.
- Works without LML for code-only release.
- Optional LML content pack later.

### Performance budget

- No full ped pool scan every frame.
- No permanent bat swarms.
- Pool particle effects and markers where possible.
- Spawn boss only when the encounter is active.
- Rate-limit ground/path checks while not aiming Shadowstep.

### Failure recovery

A watchdog should restore player visibility, collision, movement rate and input when:

- the player dies;
- a cutscene starts;
- a mission transition occurs;
- the script reloads;
- an ability throws an internal error;
- the user disables the feature.

---

# Phase 13 — Testing matrix

Test at minimum:

- Arthur and John.
- Saint Denis dense streets.
- Open plains.
- Forest.
- Steep slopes.
- Interiors.
- Water edges.
- Mounted/unmounted.
- Wanted level active.
- Story missions nearby.
- Low/high FPS.
- Controller and keyboard/mouse.

Regression checklist after every release:

- Shadowstep cannot soft-lock player.
- Speed resets.
- Feed target always releases.
- Vampire NPC despawns/cleans up correctly.
- No save corruption.
- No online support or accidental online loading.

---

# Phase 14 — Packaging and release

Release ZIP should contain only what the user needs:

```text
Nightwalker/
  Nightwalker.asi
  Nightwalker.ini
  README.txt
  CHANGELOG.md
```

Optional later content pack:

```text
lml/
  Nightwalker/
    stream/
    replace/
```

### Release stages

- `0.1` developer test — spawn + basic blink.
- `0.2` Shadowstep polished.
- `0.3` movement + hunger.
- `0.4` feeding.
- `0.5` vampire AI encounter.
- `0.6` progression/UI.
- `0.7` combat abilities.
- `0.8` narrative content.
- `0.9` compatibility/performance beta.
- `1.0` stable complete release.

---

# Recommended build order

Do **not** start with quests or a giant skill tree.

The shortest path to proving the dream is:

**Plugin boots → spawn `cs_vampire` → debug teleport → safe Shadowstep → smoke/bat polish → enemy Shadowstep → supernatural sprint → feeding → hunger → boss encounter → combat kit → progression/UI → story.**

If Shadowstep and feeding feel excellent, the project already has its identity. Everything else can grow around those two systems.
