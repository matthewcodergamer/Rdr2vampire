# Narrative and reactive conversation

Phase 12 gives the Saint Denis vampire original, coherent, non-repeating dialogue plus a contextual pre-combat interaction layer. `docs/DESIGN_LOCKS.md` remains authoritative; this does not add a new persistent combat HUD or an RPG dialogue menu.

## Ownership

`SaintDenisDirector` owns encounter state and explicitly opens/closes the conversation window. `ReactiveConversationController` owns RDR2 entity prompts and observes player context. `NarrativeController` owns one temporary authored sequence at a time. Text, speaker labels, text IDs, optional audio IDs and durations live in `Nightwalker.dialogue`, not in combat logic.

The conversation controller never scans the world. It binds only the encounter-owned `cs_vampire` exposed by `BossActorRegistry`.

## RDR2-style interaction

During Confrontation, Nightwalker registers three standard prompts into the vampire's entity-focus prompt group:

- **TALK** — plays a randomized coherent answer and returns to the choices;
- **ANTAGONIZE** — plays a randomized warning, then deliberately enters Combat;
- **LEAVE** — plays a randomized dismissal, then disengages/aborts the encounter safely.

On controller, the player uses RDR2's normal entity focus (L2/LT) to focus the vampire, then uses the face-button prompts. R2/RT is deliberately not repurposed for dialogue; it remains the normal hostile/weapon trigger, so aiming or firing can interrupt the conversation naturally.

These are native RDR2 prompt surfaces rather than a custom radial menu. The default conversation window is 22 seconds. If the player simply remains without choosing and no authored line is active when the window expires, the encounter proceeds to Combat rather than holding the free-roam scene indefinitely.

## Reactive player context

The vampire reacts to what the player actually does. Context sensing is event-driven and cooldown-limited so he does not chatter every frame.

### Firearms

- **AimStarted** — first direct aim at the vampire;
- **AimHeld** — aim remains trained on him for roughly 2.2 seconds;
- **AimLowered** — player stops aiming after threatening him;
- **RangedWeaponDrawn** — firearm is readied before an aim/firing decision;
- **ShotStarted** — immediately ends conversational courtesy and arms Combat;
- **ShotHit / ShotMiss** — first hostile shot is classified during a short bounded window and selects a matching family.

Aiming alone does not force combat. Firing does.

### Melee and unarmed

Nightwalker distinguishes the player's current weapon category and the actual melee target:

- **MeleeWeaponDrawn** — knife/hatchet/other melee-class weapon becomes current;
- **UnarmedAttackStarted** — player enters melee against the vampire with bare hands;
- **MeleeAttackStarted** — player enters melee against him with a melee weapon;
- **UnarmedHit / MeleeHit** — confirmed player contact selects a separate response family;
- starting either unarmed or armed melee against the vampire is a hostile decision and enters Combat immediately.

The controller does not infer punches from distance alone. It uses RDR2 melee-combat state plus the player's melee target.

### Other weapon posture

- **LassoDrawn** — separate reactions because trying to bind the vampire has a different dramatic meaning;
- **ThrowableDrawn** — separate reactions for thrown weapons;
- **WeaponPutAway** — restrained acknowledgement when the player returns to unarmed state after carrying a weapon.

### Movement

- **CloseApproach** — crossing into roughly 2.6 m personal space;
- **BackedAway** — after being close, retreating beyond roughly 7.5 m.

Movement reactions are more heavily cooldown-limited than direct threats.

## Coherent variation

A sequence family includes the exact family ID and child IDs separated by a dot. Examples:

```text
saint_denis.pre_fight
saint_denis.pre_fight.soul_02
saint_denis.react.aim
saint_denis.react.aim.02
saint_denis.react.melee_draw
saint_denis.react.unarmed_hit
saint_denis.react.back_away
```

`NarrativeVariantSelector` uses a non-repeating shuffle bag:

- a **whole authored sequence** is selected at once;
- individual lines are never shuffled;
- every available variant is used before the bag refills;
- the first selection after refill cannot immediately repeat the previous variant when multiple choices exist.

Context chooses the family; the shuffle bag chooses the authored response inside that family. This keeps repeated church encounters and repeated TALK/ANTAGONIZE/reactive events unpredictable without producing nonsense sentence combinations. The bag remains alive for the runtime session, so leaving the church and returning does not reset every family back to its first response.

## Voice direction

The Saint Denis vampire is calm, ancient, self-possessed and predatory. His speech should feel older through worldview and construction rather than fake Shakespearean vocabulary.

Desired dramatic traits:
- metaphysical contradiction rather than ordinary exposition;
- moral inversion rather than simple villain boasting;
- compact observations about mortality, hunger, faith and time;
- quiet amusement instead of constant growling;
- anger becomes colder and quieter, not louder;
- silence remains part of the performance.

Nightwalker may study the *dramatic grammar* of other vampire fiction, but it must not copy another game's dialogue, recordings, character identity or actor performance. All shipped dialogue and voice assets must remain original/licensed.

## Format

```text
schema=1
line=<sequence-id>|<line-id>|<speaker>|<text-id>|<audio-id>|<duration-ms>|<subtitle text>
```

The shipped external file includes the full reactive banks. A missing/corrupt external file falls back to the built-in core narrative catalog. Missing optional reaction families never block combat or cleanup; the reaction is simply silent and the underlying gameplay event still proceeds.

## Safety and cleanup

Encounter abort, player death/unsafe Story Mode transition, F10/F11 and unload disable prompts and release their handles. ANTAGONIZE/LEAVE cannot leave prompts active after the encounter exits. Shot classification is bounded and cannot hold Combat indefinitely. Weapon/movement reactions are cooldown-limited and never seize combat ownership from the existing combat controllers.

Optional audio remains isolated behind `IGameNarrativeAudioApi`. Unavailable voice assets cannot block interaction, combat or cleanup; subtitles remain the fallback.

## Configuration

```ini
[Narrative]
Enabled=true
MaxConfrontationHoldMs=6000
ConversationWindowMs=22000
MaxSequenceMs=9000
SkipKey=0x0D
OptionalAudio=true
```

`ConversationWindowMs` is clamped to 6–45 seconds. Enter skips the current subtitle line; holding Enter does not continuously advance lines.

## In-game checks

1. Enter Confrontation and hold RDR2's normal focus control on the vampire; confirm TALK / ANTAGONIZE / LEAVE appear as entity-linked RDR2 prompts rather than a custom menu.
2. Use TALK repeatedly and confirm coherent whole responses vary without immediate repetition.
3. Leave the area, allow the encounter to reset/cool down, return to the church and verify the opening family continues its shuffle bag rather than restarting the same speech.
4. Aim, hold aim, lower the gun, draw a firearm without firing, then fire; verify each relevant family can react without chatter spam.
5. Draw and put away a melee weapon; verify separate melee-draw and holster reactions.
6. Start a bare-handed fight and a melee-weapon fight; both must enter Combat immediately and choose the correct start family.
7. Land bare-hand and melee-weapon hits; verify confirmed-contact families can bark and unrelated damage does not masquerade as player contact.
8. Equip the lasso and a throwable and verify their distinct reactions.
9. Move from outside 7.5 m to inside ~2.6 m, then retreat; verify sparse approach/back-away reactions.
10. Select ANTAGONIZE; confirm its line finishes and then Combat begins.
11. Select LEAVE; confirm its line finishes, prompts disappear, and encounter cleanup/cooldown occurs with no stranded prompt.
12. Abort with F10/F11, player death or mission/cutscene transition and verify prompt/subtitle cleanup.
13. Confirm the existing red boss-health bar remains Nightwalker's only custom combat HUD.
