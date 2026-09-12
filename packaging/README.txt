NIGHTWALKER 1.0.0-rc1
Red Dead Redemption 2 Story Mode mod

IMPORTANT
---------
Nightwalker is for RDR2 STORY MODE ONLY. Do not use it in Red Dead Online.

WHAT THIS ZIP CONTAINS
----------------------
Nightwalker.asi
  The real compiled 64-bit Nightwalker plugin loaded by Script Hook RDR2.

Nightwalker.ini
  Nightwalker settings. The shipped defaults are safe starting values.

Nightwalker.dialogue
  Original Nightwalker subtitle/narrative data used by the Saint Denis encounter.

Nightwalker.asi.sha256.txt
  SHA-256 checksum for the compiled ASI so you can verify the file.

README.txt
  This quick installation guide.

README.md
  Full project instructions, controls, compatibility notes, troubleshooting and design details.

CHANGELOG.md
  Changes included in this release candidate.

THIRD_PARTY_NOTICES.md
  Dependency and redistribution notices.

REQUIREMENTS
------------
1. Red Dead Redemption 2 for Windows with Story Mode.
2. A compatible Script Hook RDR2 runtime / ASI loader installed separately.

Nightwalker does NOT bundle Script Hook RDR2 or Rockstar game assets.

INSTALLATION
------------
1. Close Red Dead Redemption 2 completely.
2. Install a compatible Script Hook RDR2 runtime according to its own instructions.
3. Open your main Red Dead Redemption 2 game folder. This is normally the folder containing RDR2.exe and the Script Hook files.
4. Copy these Nightwalker files from this ZIP into that same game folder:

   Nightwalker.asi
   Nightwalker.ini
   Nightwalker.dialogue

5. You may also keep README.txt, README.md, CHANGELOG.md, THIRD_PARTY_NOTICES.md and the SHA-256 file there for reference.
6. Start Red Dead Redemption 2 and enter STORY MODE.

FIRST RUN
---------
Nightwalker will create Nightwalker.log when file logging is available.
Nightwalker.state is created/updated by the mod for its own progression and encounter state. It does not replace or patch RDR2 save files.

WHAT HAPPENS IN GAME
--------------------
Nightwalker expands the existing Saint Denis vampire into a nighttime boss encounter.

The encounter is designed around the Saint Denis church/cathedral district during the configured midnight-to-pre-dawn window. The vampire can stalk, Shadowstep, move unnaturally fast, use close-range supernatural combat, and display the temporary red boss-health bar during active combat.

Shadowstep is the signature move: the vampire briefly disappears, safely relocates, reappears near the player with restrained dark smoke, carries forward slightly, then begins a readable attack. It is not simply extreme running speed.

The mod also includes original subtitle-first confrontation/defeat narrative. Press ENTER while a Nightwalker narrative subtitle sequence is active to advance/skip the current line.

NORMAL GAMEPLAY CONTROLS
------------------------
No custom combat ability buttons are required for the normal boss encounter.

ENTER
  Advance/skip the current Nightwalker narrative subtitle line while one is playing.

DEBUG CONTROLS
--------------
These only work if [Debug] Enabled=true in Nightwalker.ini. They are development/test controls, not the normal gameplay interface.

F1  Heavy strike test
F2  Grab/control test
F3  Release/throw follow-up
F4  Combat-feed test
F5  Sip feed test
F6  Drain feed test
F7  Player Shadowstep safety test
F8  Spawn debug vampire
F9  Despawn debug vampire
F10 Reload config/dialogue safely
F11 Global Nightwalker cleanup

HUD RULE
--------
The temporary red Saint Denis boss-health bar is Nightwalker's only custom combat HUD.
There is no blood meter, hunger meter, cooldown bar, skill wheel, boss phase label, ability-name display or floating damage UI.

TROUBLESHOOTING
---------------
If Nightwalker does not load:
1. Make sure you launched Story Mode, not Red Dead Online.
2. Confirm your Script Hook RDR2 runtime supports your installed RDR2 version.
3. Confirm Nightwalker.asi is in the folder scanned by your ASI loader, normally beside RDR2.exe.
4. Restore the shipped Nightwalker.ini if you changed settings.
5. Check Nightwalker.log for the exact initialization or runtime error.
6. Temporarily remove other gameplay mods if you suspect a conflict.

UNINSTALL
---------
Close RDR2 and remove:
  Nightwalker.asi
  Nightwalker.ini
  Nightwalker.dialogue
  Nightwalker.log
  Nightwalker.state

Nightwalker does not overwrite RDR2's proprietary save files.

RELEASE STATUS
--------------
1.0.0-rc1 is a release candidate. The ASI is built and PE-verified by the repository's Windows GitHub Actions build, but hands-on RDR2 Story Mode compatibility testing is still required before final 1.0.
