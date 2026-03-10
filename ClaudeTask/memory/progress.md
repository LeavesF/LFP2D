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
- [x] Create UMG Blueprint `WBP_PlannedSkillIcon` with UImage named `SkillIconImage`
- [ ] Set `PlannedSkillIconWidgetClass` on enemy unit blueprints
- [ ] Optionally add `ActionPointsText` TextBlock to SkillSelectionWidget UMG
- [ ] Optionally add `PhaseText` TextBlock to TurnSpeedListWidget UMG
- [x] Configure skill priority values (BasePriority, PriorityDecreaseOnUse, PriorityRecoveryPerRound) on skill Blueprint assets
- [x] Configure faction AP values (FactionMaxAP, FactionInitialAP, FactionAPRecovery) on TurnManager Blueprint
- [x] Create terrain DataAsset instances (DA_Terrain_Grass created in Content/Grid/Terrain/)
- [ ] Set DefaultTerrainData on BP_HexGridManager
- [ ] Configure specific tiles with different terrain types for testing
- [x] Create WBP_MapEditor UMG Blueprint in Content/UI/MapEditor/
- [x] Create IA_ToggleEditor InputAction (e.g. F5) and add to input mapping
- [x] Configure TerrainRegistry on BP_HexGridManager (map each ELFPTerrainType → DA_Terrain_* asset)
- [x] Configure DecorationRegistry on BP_HexGridManager (map FName → decoration sprites)

## Planned Features

### World Map System
#### Phase 1+2: 数据层 + 可视化（完成）
- [x] LFPWorldMapData.h: ELFPWorldNodeType 枚举（7 种节点）、FLFPWorldNodeRow、FLFPWorldEdgeRow
- [x] LFPWorldNodeDataAsset.h: 节点类型视觉数据资产（DefaultSprite/TriggeredSprite/FogSprite）
- [x] ALFPWorldMapNode Actor: 数据 + PaperSpriteComponent + 高亮 + InitFromRowData/ExportToRowData
- [x] ALFPWorldMapEdge Actor: 数据 + 拉伸精灵线段 + UpdateVisualPosition
- [x] ALFPWorldMapManager: NodeMap/EdgeMap/AdjacencyList、SpawnNode/RemoveNode/AddEdge/RemoveEdge、CSV 保存/加载、NodeVisualRegistry

#### Phase 3: 世界地图编辑器（完成）
- [x] LFPWorldMapEditorComponent（工具模式、笔刷参数、嵌套编辑）
- [x] LFPWorldMapEditorWidget（UI）
- [x] PlayerController 集成（输入路由、Toggle）

#### GameMode 架构重构（完成）
- [x] ALFPWorldMapGameMode（世界地图专用 GameMode，生成 WorldMapManager）
- [x] ALFPWorldMapPlayerController（世界地图专用 PlayerController，相机/编辑器/节点交互）
- [x] 从 ALFPTacticsPlayerController 移除世界地图相关代码

#### Phase 4: 玩家移动 + 迷雾（完成）
- [x] LFPWorldMapPlayerState（CurrentNodeID、CurrentTurn、VisitedNodeIDs、RevealedNodeIDs、MoveToNode、GetReachableNodeIDs）
- [x] Manager 迷雾系统（BFS GetNodesWithinGraphDistance、UpdateFog、InitializePlayer、MovePlayer）
- [x] PlayerController 世界地图移动逻辑（MoveToNode、ShowReachableNodes、ClearReachableHighlight、EnterNode）
- [x] 回合压力系统（TurnPressureThreshold=100、IsPastPressureThreshold）
- [x] 节点触发标记（战斗/事件/Boss 首次触发后标记，商店/城镇可反复进入）
- [x] 前置节点解锁检查（PrerequisiteNodeIDs 分号分隔解析）

#### Phase 5: 场景切换（完成）
- [x] ULFPGameInstance（跨关卡状态中转：FLFPBattleRequest、FLFPBattleResult、FLFPWorldMapSnapshot）
- [x] WorldMapPlayerController::EnterNode 按节点类型分发（战斗/事件/商店等）
- [x] WorldMapPlayerController::EnterBattle（保存快照 + 设置战斗请求 + OpenLevel）
- [x] TurnGameMode::StartPlay 读取 BattleRequest（ConsumeBattleRequest）
- [x] TurnGameMode 生成 HexGridManager + 从 CSV 加载地图 + 生成 TurnManager
- [x] TurnGameMode::EndBattle（写回 BattleResult + TransitionToWorldMap）
- [x] WorldMapGameMode::StartPlay 从快照恢复（加载地图 + 恢复 PlayerState + 已触发节点 + 处理战斗结果）
- [x] Manager: GetCurrentWorldMapName/SetCurrentWorldMapName、RestoreTriggeredNodes、LoadWorldMap 记录地图名

#### Phase 6: 世界地图玩家棋子（完成）
- [x] ALFPWorldMapPawn: PaperSpriteComponent + FTimeline 移动动画 + OnMoveComplete 委托
- [x] Manager 集成: SpawnPawn、MoveToLocation、HandlePostMove

#### Phase 7: 队伍/备战营 + 布置阶段（完成）
- [x] FLFPUnitEntry + ULFPUnitRegistryDataAsset 单位数据层
- [x] GameInstance 编队管理（PartyUnits/ReserveUnits/TryAddUnit/Replace/Remove）
- [x] 战斗捕获流程（ChangeAffiliation → RecordCapturedUnit → EndBattle → ProcessCapturedUnits）
- [x] ULFPUnitReplacementWidget（溢出替换 UI）
- [x] BP_Deployment 阶段（EBattlePhase 新增）
- [x] TurnManager 布置阶段流程（StartGame 仅注册敌方 → 设置 BP_Deployment → EndDeploymentPhase 注册玩家）
- [x] PlayerController 布置交互（高亮出生点、放置/拾起/确认）
- [x] LFPDeploymentWidget（BindWidget 模式，C++ 逻辑）
- [x] 时序修复：PlayerController::BeginPlay 主动检查阶段而非依赖委托广播

## Known Issues / Notes
- `FindBestCasterPosition` and `SelectBestSkill` cannot be `const` because `GetCurrentTile()` is non-const
- SkillBase `EvaluateConditionBonus()` returns 0 by default — override in Blueprint subclasses for context-aware priority bonuses
- AP is consumed during AllocateEnemySkills (planning), NOT during action phase execution
- HexGridManager::BeginPlay() 中原有自动加载逻辑已注释掉，改为由 TurnGameMode 控制生成和加载
- TurnManager 是 GameMode 的成员，不再使用局部变量
