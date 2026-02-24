# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LFP2D is a 2D turn-based tactical RPG built with Unreal Engine 5.5 using Paper2D for rendering. It features hexagonal grid-based movement, a speed-based turn system, skill/combat mechanics, AI opponents via behavior trees, and a unit betrayal system.

## Build & Development

This is an Unreal Engine 5.5 C++ project. The single runtime module is **LFP2D**.

**Build from command line (Windows):**
```
"<UE5_Install>/Engine/Build/BatchFiles/Build.bat" LFP2D Win64 Development "D:/UE/UE_Project/LFP2D/LFP2D.uproject" -waitmutex
```

**Editor build:**
```
"<UE5_Install>/Engine/Build/BatchFiles/Build.bat" LFPEditor Win64 Development "D:/UE/UE_Project/LFP2D/LFP2D.uproject" -waitmutex
```

**Key dependencies** (from LFP2D.Build.cs): Core, CoreUObject, Engine, InputCore, EnhancedInput, Paper2D, SlateCore, GameplayTags.

**Plugins:** CommonUI (Runtime), ModelingToolsEditorMode (Editor only).

## Architecture

### Core Systems (Source/LFP2D/)

- **Core/** — `LFPTurnGameMode`: Game mode that initializes the turn system at play start.
- **Turn/** — `LFPTurnManager`: Manages speed-based turn ordering, round cycles, unit registration. Fires `OnTurnChanged` delegate.
- **Player/** — `LFPTacticsPlayerController`: Central player controller handling Enhanced Input, camera control (pan/drag/zoom), unit selection, movement commands, and skill execution. Manages game states (MoveState, SkillReleaseState).
- **HexGrid/** — `LFPHexGridManager` + `LFPHexTile`: Hex grid generation using cube coordinates (Q, R, S), A* pathfinding, range calculations, tile occupancy/walkability, and sprite layers for highlighting (movement, attack, skill effect, path).
- **Unit/** — `LFPTacticsUnit`: Core unit actor with stats (HP, Attack, Defense, Movement Range, Action Points, Speed), affiliation (Player/Enemy/Neutral), timeline-based movement animation, PaperSprite rendering, SkillComponent and WidgetComponent attachments.
- **Skill/** — `LFPSkillBase` (abstract base), `LFPSkillComponent` (per-unit skill manager), `LFPSkillDataAsset` (data-driven skill definitions), `LFPSkillButtonWidget` (UI). Skills define hex-coordinate patterns for release range and effect area, support multiple targeting types (Self, SingleAlly, SingleEnemy, Area variants, Tile), have cooldown and action point costs.
- **AI/** — `LFPAIController` with behavior tree integration. `LFPEnemyBehaviorData` data asset defines AI personality (aggressiveness, defensiveness, teamwork). BT tasks in `AI/BehaviorTree/`: FindTarget, FindMovementTile, MoveToTile, AttackTarget, EndTurn, plus UpdateConditions service.
- **Unit/Betrayal/** — `LFPBetrayalManager`, `LFPBetrayalCondition` (abstract), `LFPBCondition_HPLack` (low-HP trigger), `LFPUnitBetrayalData`. Framework for runtime affiliation changes.
- **UI/Fighting/** — `LFPSkillSelectionWidget` (skill panel with grid layout, filtering, sorting), `LFPTurnSpeedListWidget`/`LFPTurnSpeedIconWidget` (turn order display), `LFPHealthBarWidget`.

### Game Flow

```
TurnManager (speed-based turn order)
  → PlayerController (for player units: input → select action)
  → AIController (for enemy units: behavior tree → find target → move → attack)
    → TacticsUnit.MoveToTile() / SkillComponent.ExecuteSkill()
      → HexGridManager (pathfinding, range queries)
      → UI updates via delegates (OnHealthChanged, OnSkillExecuted, etc.)
```

### Design Patterns

- **Component-based**: Units attach SkillComponent, HealthBar WidgetComponent
- **Event-driven**: Multicast delegates for turn changes, health changes, skill execution, death
- **Data-driven**: Skills and AI behavior configured via UDataAsset subclasses and Blueprints
- **Behavior trees**: AI decision-making with custom BTTask and BTService nodes

### Content Organization

- `Content/Battle/Skill/Skills/` — Blueprint skill implementations (e.g., BP_Skill_TestArrow, BP_Skill_TestRecover)
- `Content/Map/2DMap.umap` — Main battle map
- `Content/Core/BP_TacticsPC` — Player controller Blueprint
- `Content/AI/` — Behavior tree assets
- `Content/UI/Fighting/` — Battle UI widget Blueprints
- `Content/Input/` — Enhanced Input mapping configs

## Code Conventions

- Class prefix: `LFP` (e.g., `ALFPTacticsUnit`, `ULFPSkillBase`, `ALFPHexTile`)
- Header includes use `#pragma once`
- Uses Enhanced Input System (not legacy input)
- Skills are defined as Blueprint subclasses of `ULFPSkillBase` with `Execute()` implemented in Blueprint
- Hex coordinates use cube coordinate struct `FLFPHexCoordinates` with Q, R, S fields
