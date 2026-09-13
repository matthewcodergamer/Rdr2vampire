# Narrative and reactive conversation

Phase 12 gives the Saint Denis vampire original, coherent, non-repeating dialogue plus a contextual pre-combat interaction layer. `docs/DESIGN_LOCKS.md` remains authoritative; this does not add a new persistent combat HUD or an RPG dialogue menu.

## Ownership

`SaintDenisDirector` owns encounter state and explicitly opens/closes the conversation window. `ReactiveConversationController` owns RDR2 entity prompts and observes player context. `NarrativeController` owns one temporary authored sequence at a time. Text, speaker labels, text IDs, optional audio IDs and durations live in `Nightwalker.dialogue`, not in combat logic.

The conversation controller never scans the world. It binds only the encounter-owned `cs_vampire` exposed by `BossActorRegistry`.

## RDR2-style interaction

During Confrontation, Nightwalker registers three standard prompts into the vampire's entity-focus prompt group:

- **QUESTION** — plays a randomized coherent answer and returns to the choices;
- **CHALLENGE** — plays a randomized warning, then deliberately enters Combat;
- **LEAVE** — plays a randomized dismissal, then disengages/aborts the encounter safely.

These are native RDR2 prompt surfaces rather than a custom radial menu. The weapon trigger is not stolen for dialogue. A player can still aim/fire normally.

The default conversation window is 22 seconds. If the player simply remains without choosing and no authored line is active when the window expires, the encounter proceeds to Combat rather than holding the free-roam scene indefinitely.

## Reactive weapon context

The vampire reacts to player behavior, not just menu choices:

- **AimStarted** — first time the player aims directly at him;
- **AimHeld** — the weapon remains trained on him for roughly 2.2 seconds;
- **AimLowered** — the player stops aiming after threatening him;
- **ShotStarted** — immediately ends conversational courtesy and arms Combat;
- **ShotHit / ShotMiss** — the first hostile shot is classified during a short bounded window and selects a matching bark family.

Aiming alone does not force combat. Firing does. Weapon-context lines may interrupt a philosophical pre-fight line because the world state has materially changed.

The first hostile shot keeps only enough reactive ownership to classify hit versus miss after Combat has been armed; all choice prompts remain hidden. Normal combat thereafter is owned by the existing AI/combat controllers rather than by the conversation system.

## Coherent variation

A sequence family includes the exact family ID and child IDs separated by a dot. Examples:

```text
saint_denis.pre_fight
saint_denis.pre_fight.soul_02
saint_denis.react.aim
saint_denis.react.aim.02
saint_denis.react.shot_hit
saint_denis.react.shot_hit.02
```

`NarrativeVariantSelector` uses a non-repeating shuffle bag:

- a **whole authored sequence** is selected at once;
- individual lines are never shuffled;
- every available variant is used before the bag refills;
- the first selection after refill cannot immediately repeat the previous variant when multiple choices exist.

The same rule applies independently to soul speeches, questions, challenges, leave responses, aiming, aim-held, weapon-lowered, hit and miss families. Context chooses the family; the shuffle bag chooses the authored conversation inside that family.

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

The shipped external file includes the full reactive banks. A missing/corrupt external file falls back to the built-in core narrative catalog. If optional reactive families are absent, QUESTION/aim barks may be silent, while CHALLENGE/LEAVE still preserve their gameplay intent rather than trapping the encounter.

## Safety and cleanup

Encounter abort, player death/unsafe Story Mode transition, F10/F11 and unload disable prompts and release their handles. Challenge/Leave cannot leave prompts active after the encounter exits. First-shot classification is bounded and cannot hold Combat indefinitely.

Optional audio remains isolated behind `IGameNarrativeAudioApi`. The current backend is subtitle-first, so unavailable voice assets cannot block interaction, combat or cleanup.

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

1. Enter Confrontation and focus the vampire; confirm QUESTION / CHALLENGE / LEAVE appear as entity-linked RDR2 prompts rather than a custom menu.
2. Use QUESTION several times and confirm coherent whole responses vary without immediate repetition.
3. Aim at the vampire during a soul line; confirm the line can be interrupted by an aiming response without immediately starting combat.
4. Hold aim beyond ~2.2 seconds and verify one aim-held response, not repeated spam.
5. Lower the weapon and verify a restrained lowered-weapon response.
6. Fire while dialogue is active; confirm dialogue stops and Combat arms immediately.
7. Test a first shot that hits and one that misses; confirm the correct reaction family can bark over the opening of Combat.
8. Select CHALLENGE; confirm its line finishes and then Combat begins.
9. Select LEAVE; confirm its line finishes, prompts disappear, and encounter cleanup/cooldown occurs with no stranded prompt.
10. Abort with F10/F11, player death or mission/cutscene transition and verify prompt/subtitle cleanup.
11. Confirm the existing red boss-health bar remains Nightwalker's only custom combat HUD.
