# LFP2D Project Memory

## User Preferences
- Communicates in Chinese, code comments in Chinese
- Git commit style: `[*]` for changes, `[+]` for additions, `[-]` for removals
- Prefers concise explanations, doesn't want unnecessary commits
- Wants to be consulted on design decisions before implementation

## Architecture Decisions Log
See [architecture.md](architecture.md) for detailed system design records.

## Key File Paths
- Battle types (shared enums/structs): `Source/LFP2D/Turn/LFPBattleTypes.h`
- Turn manager: `Source/LFP2D/Turn/LFPTurnManager.h/.cpp`
- AI controller: `Source/LFP2D/AI/LFPAIController.h/.cpp`
- Skill base: `Source/LFP2D/Skill/LFPSkillBase.h/.cpp`
- Skill component: `Source/LFP2D/Skill/LFPSkillComponent.h/.cpp`
- Tactics unit: `Source/LFP2D/Unit/LFPTacticsUnit.h/.cpp`
- Player controller: `Source/LFP2D/Player/LFPTacticsPlayerController.h/.cpp`
- Skill selection UI: `Source/LFP2D/UI/Fighting/LFPSkillSelectionWidget.h/.cpp`
- Planned skill icon: `Source/LFP2D/UI/Fighting/LFPPlannedSkillIconWidget.h/.cpp`

## Current Development Status
See [progress.md](progress.md) for implementation progress.
