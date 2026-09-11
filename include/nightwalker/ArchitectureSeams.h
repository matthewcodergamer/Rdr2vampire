#pragma once

// Phase 0 forward declarations only. Concrete systems are added in their owning phase.
namespace nightwalker::core {
class Config;
class SafetyWatchdog;
class SaveData;
}
namespace nightwalker::systems {
class ShadowstepController;
class MovementController;
class FeedingController;
class VampireAIController;
class EncounterDirector;
}
namespace nightwalker::ui {
class BossHudController;
}
