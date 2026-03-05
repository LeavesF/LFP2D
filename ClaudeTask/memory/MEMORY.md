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
- Map data struct: `Source/LFP2D/HexGrid/LFPMapData.h`
- Terrain data asset: `Source/LFP2D/HexGrid/LFPTerrainDataAsset.h`
- Map editor component: `Source/LFP2D/HexGrid/LFPMapEditorComponent.h/.cpp`
- Map editor widget: `Source/LFP2D/UI/MapEditor/LFPMapEditorWidget.h/.cpp`
- World map data: `Source/LFP2D/WorldMap/LFPWorldMapData.h`
- World map node: `Source/LFP2D/WorldMap/LFPWorldMapNode.h/.cpp`
- World map edge: `Source/LFP2D/WorldMap/LFPWorldMapEdge.h/.cpp`
- World map manager: `Source/LFP2D/WorldMap/LFPWorldMapManager.h/.cpp`
- World node data asset: `Source/LFP2D/WorldMap/LFPWorldNodeDataAsset.h`
- World map editor component: `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h/.cpp`
- World map editor widget: `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h/.cpp`
- World map game mode: `Source/LFP2D/WorldMap/LFPWorldMapGameMode.h/.cpp`
- World map player controller: `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h/.cpp`
- World map player state: `Source/LFP2D/WorldMap/LFPWorldMapPlayerState.h/.cpp`
- Game instance: `Source/LFP2D/Core/LFPGameInstance.h/.cpp`

## Current Development Status
See [progress.md](progress.md) for implementation progress.
