#include "nightwalker/game/GameFeedingApi.h"

// Intentionally empty production boundary.
//
// RDR3 native references document front/rear style names for
// TASK_PUT_PED_DIRECTLY_INTO_GRAPPLE, but Nightwalker does not yet have enough
// target-environment evidence for that native's remaining parameters to make it
// a production dependency. VampireFeedPresentation therefore falls back to the
// existing verified TASK_GRAPPLE path. Keeping this translation unit explicit
// makes the research seam visible without guessing a native contract.
