# Tests

Phase 0 has no gameplay or pure state/math behavior worth fabricating a test around. Its automated gate is successful Debug/Release x64 compilation with the official Script Hook RDR2 developer SDK present.

As pure logic is introduced, tests belong here and should avoid requiring a running RDR2 process whenever practical. Candidate targets include landing-selection math, state transitions, cooldown timing, boss HUD visibility timing, and config/save migration.
