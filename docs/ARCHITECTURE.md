# Runtime Architecture

## Production target

**Windows x64, Red Dead Redemption 2 Story Mode, native `.asi` plugin.**

Primary runtime dependency: **Script Hook RDR2**.

Optional content dependency later: **Lenny's Mod Loader (LML)** only if we begin streaming original textures, audio, map additions or other content that cannot be delivered cleanly through the script plugin.

---

## Repository layout

Planned structure:

```text
/
├─ README.md
├─ docs/
│  ├─ ROADMAP.md
│  ├─ ARCHITECTURE.md
│  └─ SHADOWSTEP.md
├─ config/
│  └─ Nightwalker.example.ini
├─ src/
│  ├─ Main.cpp
│  ├─ Core/
│  │  ├─ Config.*
│  │  ├─ Logger.*
│  │  ├─ SaveData.*
│  │  └─ GameClock.*
│  ├─ Player/
│  │  ├─ VampireState.*
│  │  ├─ ShadowstepController.*
│  │  ├─ MovementController.*
│  │  └─ FeedingController.*
│  ├─ AI/
│  │  ├─ VampireAIController.*
│  │  └─ EncounterDirector.*
│  ├─ Presentation/
│  │  ├─ VfxController.*
│  │  ├─ AudioController.*
│  │  └─ HudController.*
│  └─ World/
│     └─ SaintDenisEncounter.*
└─ third_party/
   └─ README.md
```

Do not commit Rockstar game assets or third-party binaries simply because they are installed locally.

---

## Main update loop

The main Script Hook fiber/tick should be thin:

```text
Poll game state
→ update safety watchdog
→ update clock/form
→ update input
→ update active player ability state machine
→ update encounter director at throttled frequency
→ update active vampire AI
→ draw minimal HUD/debug info
→ yield
```

### Tick frequencies

**Every frame**
- active input;
- active Shadowstep transition;
- immediate combat state;
- HUD marker while aiming.

**~10–20 Hz**
- nearby feed-target refresh;
- boss decision updates;
- VFX cleanup.

**~1–2 Hz**
- encounter spawn checks;
- broad nearby-ped queries;
- hunger passive decay;
- save dirty-state checks.

This keeps the mod responsive without doing expensive world work 60+ times per second.

---

## Player vampire state

```text
Human / Disabled
  ↓ enable or night rule
VampireIdle
  ├─ Sprinting
  ├─ Shadowstep
  ├─ Feeding
  ├─ CombatAbility
  ├─ Stunned/Ragdoll
  └─ Mounted / Restricted
```

Only one high-priority action owns the player at a time.

Priority example:

1. death/cutscene cleanup;
2. mission restriction;
3. ragdoll;
4. feeding paired animation;
5. Shadowstep;
6. combat ability;
7. supernatural sprint;
8. idle.

---

## Hunger data

Suggested structure:

```cpp
struct VampireStats {
    float hunger = 100.0f;
    float maxHunger = 100.0f;
    float shadowstepCooldownMultiplier = 1.0f;
    float shadowstepRangeMultiplier = 1.0f;
    float sprintMultiplier = 1.0f;
    int progressionPoints = 0;
};
```

All values should be clamped and save-file versioned.

---

## Shadowstep state

```cpp
enum class ShadowstepState {
    Idle,
    Targeting,
    Validating,
    Departure,
    Transit,
    Arrival,
    Recovery,
    Cooldown
};
```

Data carried by one cast:

```cpp
struct ShadowstepCast {
    Vector3 origin;
    Vector3 requestedDestination;
    Vector3 safeDestination;
    Entity target;
    bool combatTargeted;
    bool collisionWasEnabled;
    int startedAtMs;
};
```

Never store raw entity handles forever; revalidate entities before using them.

---

## Encounter director

Each encounter has:

```text
Inactive
→ Eligible
→ Omen
→ Spawn
→ Stalking
→ Conversation / Aggro
→ Combat
→ Resolved
→ Cooldown
```

Eligibility combines:

- world time;
- player distance;
- story/mission safety;
- encounter cooldown;
- whether the player is already in combat;
- whether the spawn point is off-camera and unoccupied.

The first major encounter uses RDR2's `cs_vampire` model at runtime rather than bundling that model in the mod.

---

## Save format

Use a separate file, for example:

```text
Nightwalker.save.json
```

Version it:

```json
{
  "version": 1,
  "vampireEnabled": true,
  "hunger": 84.2,
  "progressionPoints": 3,
  "unlocks": ["shadowstep", "feed"],
  "encounters": {
    "saintDenisVampire": {
      "completed": false,
      "cooldownUntil": 0
    }
  }
}
```

Never patch the game's own save structure.

---

## Content boundaries

### Safe for repository

- original source code;
- configs;
- documentation;
- original textures/audio/models we own or have permission to redistribute;
- hashes/names used to reference assets already installed in RDR2.

### Do not put in repository

- RDR2 game files;
- ripped Dawnwalker assets;
- extracted commercial voice acting/music;
- third-party mod binaries without permission.

---

## Crash/soft-lock watchdog

Create a global cleanup routine callable from every subsystem:

```text
Restore player visibility
Restore alpha
Restore collision
Restore movement speed
Release input locks
Stop mod-owned looping FX
Detach temporary entities
Clear mod-owned paired animation state
Delete mod-spawned temporary bats/peds
Return state machines to Idle
```

Run it on script shutdown and whenever the game enters an unexpected state.

---

## Why native C++ first

A C++ `.asi` build gives us direct access to Script Hook RDR2 and the native function set, which matters for:

- precise per-frame movement;
- shape tests;
- animation and entity state;
- AI control;
- low-overhead VFX;
- future low-level compatibility work.

A managed/C# prototype could be useful for experiments, but the long-term target should remain the native plugin so the core mechanic is not constrained by a wrapper API.
