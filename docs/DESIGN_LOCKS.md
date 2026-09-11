# Nightwalker — Locked Design Rules

These rules are project-level constraints. If any older roadmap text conflicts with this file, **this file wins** until the owner explicitly changes it.

## 1. HUD rule: boss bar only

Nightwalker must **not** add a permanent custom HUD for the player.

Do not add:

- player blood/hunger meters;
- Shadowstep cooldown meters;
- vampire-state icons;
- power names or ability cards during combat;
- boss phase labels;
- boss ability names;
- enemy weakness/resistance panels;
- skill-wheel HUD;
- floating damage numbers;
- combo counters;
- MMO-style status icons;
- tutorial overlays that reveal what the vampire boss can do.

The supernatural experience should be learned from animation, sound, VFX, enemy behavior, environment and combat itself.

The **only custom persistent combat UI element** is a minimal red boss-health meter for the Saint Denis vampire encounter. It is temporary, not permanently pinned to the screen.

Internal systems such as hunger, cooldowns, state machines and progression may exist in code if the design requires them, but they must not automatically expose themselves as custom HUD.

## 2. Boss mystery rule

Never reveal the boss's powers before he uses them.

The UI must not say things such as:

- `Shadowstep ready`;
- `Phase 2`;
- `Blood Feed`;
- `Regeneration active`;
- `Weak to ...`;
- cooldown values;
- move lists.

The player should discover the vampire by fighting him.

## 3. Visual direction

Keep RDR2's grounded visual language.

- Shadowstep is extremely fast disappearance + relocation + short arrival carry, not a giant magical explosion.
- Departure and arrival effects should be compact, dark, smoky and readable.
- Bats are accent effects, not permanent swarms.
- Red is reserved mainly for blood, danger and the boss-health bar.
- Avoid neon fantasy UI.
- Avoid screen-filling particles that obscure melee combat.

## 4. Existing-game-first rule

Prefer runtime references to content already present in the player's legitimate RDR2 installation rather than redistributing Rockstar assets.

Known useful anchor:

- `cs_vampire` / hash `0xD95BCB7D` for the existing Saint Denis vampire character.

Investigate available RDR2 animations, particles, sounds, props and animal archetypes at runtime. Do not package extracted copyrighted game assets unless redistribution is clearly permitted.

## 5. Dawnwalker inspiration boundary

We may study the *feel and combat grammar* of modern vampire games: rapid vanish/reposition, predatory speed, threatening close-range reappearance, feeding, dark smoke and cinematic boss presentation.

Do **not** ship copied Dawnwalker code, dialogue recordings, music, character models, proprietary animations or ripped assets. Original dialogue and properly licensed/original audio only.

## 6. Story Mode only

Nightwalker is a single-player Story Mode project.

- Do not target RDR Online.
- Do not add network hooks, matchmaking or online gameplay support.
- Every feature must fail safely if Story Mode state is not valid.

## 7. Safety/cleanup rule

Any feature that changes visibility, collision, invulnerability, movement rate, controls, camera, entity attachments or AI state must have a guaranteed cleanup path.

Cleanup must run on at least:

- player death;
- boss death;
- mission/cutscene transition;
- fast travel or major world transition;
- script reload/unload;
- encounter abort;
- internal error/timeout;
- configuration disable.

No feature is considered complete until it cannot strand the player invisible, collisionless, frozen, attached to another ped or permanently speed-modified.

## 8. Build-order rule

Do not build the whole dream in one giant change. Progress in verified slices:

1. plugin boots;
2. logging/config/safety cleanup;
3. spawn and control `cs_vampire` in a debug encounter;
4. safe Shadowstep prototype;
5. Shadowstep presentation and combat targeting;
6. vampire boss AI;
7. supernatural movement;
8. feeding/blood systems;
9. expanded combat;
10. Saint Denis encounter direction;
11. boss-health bar;
12. progression/story/persistence as desired;
13. compatibility, performance and release polish.

Each phase must remain runnable before the next phase begins.
