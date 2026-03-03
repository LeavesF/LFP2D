# Implementation Progress

## Completed Features

### Two-Phase Battle System
- [x] EBattlePhase enum, FEnemyActionPlan struct in LFPBattleTypes.h
- [x] TurnManager phase state machine (SetPhase, BeginEnemyPlanningPhase, BeginActionPhase)
- [x] AIController: CreateActionPlan, SelectBestSkill, FindBestCasterPosition
- [x] TacticsUnit: SetActionPlan, ClearActionPlan, ShowPlannedSkillIcon
- [x] LFPPlannedSkillIconWidget created
- [x] PlayerController: hover preview (ShowEnemyPlanPreview/HideEnemyPlanPreview), OnPhaseChanged
- [x] TurnSpeedListWidget: phase text display

### Skill Check Refactoring
- [x] IsAvailable() in SkillBase
- [x] CanReleaseFrom() (BlueprintNativeEvent) in SkillBase
- [x] CanExecute() refactored to full check
- [x] AIController updated to use IsAvailable/CanReleaseFrom

### Faction AP + Skill Priority
- [x] SkillBase: priority properties + OnSkillUsed/RecoverPriority/GetEffectivePriority
- [x] TurnManager: faction AP management (TMap, Get/Has/Consume, AP recovery in BeginNewRound)
- [x] TurnManager: AllocateEnemySkills (priority-based global allocation)
- [x] TurnManager: two-step planning flow (allocate then move by speed)
- [x] TacticsUnit: removed per-unit AP, proxy to TurnManager
- [x] AIController: CreateActionPlan accepts PreAllocatedSkill parameter
- [x] SkillComponent: calls OnSkillUsed after Execute
- [x] SkillSelectionWidget: ActionPointsText (BindWidgetOptional), OnFactionAPChanged binding

### Bug Fixes & Refactoring
- [x] SkillBase: added `InitSkill(ALFPTacticsUnit*)` — fixes BP default values (e.g. BasePriority) not applied in constructor
- [x] SkillBase: removed `OwnerController` property, Owner now set via InitSkill
- [x] SkillComponent: simplified InitializeSkills — single `InitSkill(OwnerUnit)` call replaces manual setup
- [x] SkillSelectionWidget: renamed to `InitializeSkillsInfo`, fixed duplicate delegate binding (RemoveDynamic before AddDynamic)
- [x] SkillComponent: DefaultAttackSkill assignment removed (feature deferred, property kept)

### Terrain Type System
- [x] ELFPTerrainType enum in LFPBattleTypes.h (9 terrain types)
- [x] ULFPTerrainDataAsset (header-only DataAsset): MovementCost, bIsWalkable, DefaultSprite, DisplayName
- [x] ALFPHexTile: TerrainData property, GetMovementCost(), SetTerrainData(), DecorationSpriteComponent
- [x] ALFPHexGridManager: DefaultTerrainData, GenerateGrid applies terrain, GetTilesInRange BFS→Dijkstra, FindPath A* variable cost
- [x] ALFPTacticsUnit: MoveToTile/FinishMove use actual path cost instead of path length

### Data-Driven Map System + In-Game Map Editor
- [x] FLFPMapTileRow (FTableRowBase) + ELFPSpawnFaction in LFPMapData.h
- [x] ALFPHexTile: SpawnFaction, SpawnIndex, EventTag, DecorationID, SetDecorationByID()
- [x] ALFPHexGridManager: TerrainRegistry, DecorationRegistry, MapDataTable
- [x] ALFPHexGridManager: SpawnTileAt, ClearGrid, AddTile, RemoveTile
- [x] ALFPHexGridManager: LoadMapFromDataTable, LoadMapFromCSV, SaveMapToCSV, ExportMapData, GetSpawnPoints
- [x] GenerateGrid refactored (uses SpawnTileAt), BeginPlay supports MapDataTable
- [x] ULFPMapEditorComponent: tool modes, brush params, Save/Load/NewMap
- [x] ULFPMapEditorWidget: BindWidgetOptional UI with tool buttons, dropdowns, file I/O
- [x] PlayerController: ToggleEditorAction, editor mode input routing, Z=0 plane raycast

## Editor TODO (manual)
- [ ] Create UMG Blueprint `WBP_PlannedSkillIcon` with UImage named `SkillIconImage`
- [ ] Set `PlannedSkillIconWidgetClass` on enemy unit blueprints
- [ ] Optionally add `ActionPointsText` TextBlock to SkillSelectionWidget UMG
- [ ] Optionally add `PhaseText` TextBlock to TurnSpeedListWidget UMG
- [x] Configure skill priority values (BasePriority, PriorityDecreaseOnUse, PriorityRecoveryPerRound) on skill Blueprint assets
- [ ] Configure faction AP values (FactionMaxAP, FactionInitialAP, FactionAPRecovery) on TurnManager Blueprint
- [x] Create terrain DataAsset instances (DA_Terrain_Grass created in Content/Grid/Terrain/)
- [ ] Set DefaultTerrainData on BP_HexGridManager
- [ ] Configure specific tiles with different terrain types for testing
- [ ] Create WBP_MapEditor UMG Blueprint in Content/UI/MapEditor/
- [ ] Create IA_ToggleEditor InputAction (e.g. F5) and add to input mapping
- [ ] Configure TerrainRegistry on BP_HexGridManager (map each ELFPTerrainType → DA_Terrain_* asset)
- [ ] Configure DecorationRegistry on BP_HexGridManager (map FName → decoration sprites)

## Planned Features

### World Map System (design complete, not yet implemented)
- [ ] 数据结构：节点（FLFPWorldMapNodeData）、边（FLFPWorldMapEdgeData）、序列化为 CSV/DataTable
- [ ] ALFPWorldMapNode Actor（Sprite 渲染节点类型）
- [ ] ALFPWorldMapEdge Actor（边渲染）
- [ ] ALFPWorldMapManager（节点/边管理、迷雾逻辑、数据加载/保存）
- [ ] 关卡地图编辑器（外层，放置/连接节点）
- [ ] 嵌套编辑：关卡编辑器 → 战斗地图编辑器的切换
- [ ] 场景切换：Level Streaming（关卡地图 ↔ 战斗场景）
- [ ] 玩家移动逻辑（消耗回合、触发节点事件）
- [ ] 迷雾系统（图距离视野范围，永久揭开）
- [ ] 回合压力系统（100 回合后全局难度递增事件）

## Known Issues / Notes
- `FindBestCasterPosition` and `SelectBestSkill` cannot be `const` because `GetCurrentTile()` is non-const
- SkillBase `EvaluateConditionBonus()` returns 0 by default — override in Blueprint subclasses for context-aware priority bonuses
- AP is consumed during AllocateEnemySkills (planning), NOT during action phase execution
- UE BP default values are applied AFTER C++ constructor — use InitSkill() pattern for runtime state that depends on BP config
