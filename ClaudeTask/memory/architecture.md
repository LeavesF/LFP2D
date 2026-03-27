# Architecture Decisions

## 1. Two-Phase Battle System (completed)
Replaced simple round-robin turns with:
- **EnemyPlanning phase**: enemies choose skills (by hatred), select targets, move to cast positions, display planned skill icons above heads. Player can hover to preview effect ranges.
- **Action phase**: all units act by speed. Player units get normal input; enemy units auto-execute pre-planned skills at pre-planned positions (even if target died).
- Deployment phase explicitly deferred to later.

Key types in `LFPBattleTypes.h`:
- `EBattlePhase`: BP_EnemyPlanning, BP_ActionPhase, BP_RoundEnd
- `FEnemyActionPlan`: stores EnemyUnit, PlannedSkill, TargetUnit, TargetTile, CasterPositionTile, EffectAreaTiles
- `EUnitAffiliation`: UA_Player, UA_Enemy, UA_Neutral (moved here from TacticsUnit.h to avoid circular deps)

## 2. Skill Check Three-Layer Split (completed)
Split ambiguous `CanExecute()` into:
- `IsAvailable()` — cooldown + AP check (AI skill selection)
- `CanReleaseFrom(CasterTile, TargetTile)` — range check, BlueprintNativeEvent (AI positioning)
- `CanExecute(TargetTile)` — full pre-execution check (player use)

## 3. Faction AP System (completed)
AP is per-faction (shared pool), not per-unit:
- Config: FactionInitialAP=1, FactionMaxAP=3, FactionAPRecovery=1/round
- TurnManager manages `TMap<EUnitAffiliation, int32> FactionCurrentAP`
- TacticsUnit's `HasEnoughActionPoints()`, `ConsumeActionPoints()`, `GetActionPoints()` all proxy to TurnManager
- Removed `MaxActionPoints` and `CurrentActionPoints` from TacticsUnit

## 4. Skill Priority System (completed)
Ensures fair AP allocation (not just fastest enemy):
- Each skill has `SkillPriority` (current), `BasePriority` (default=50), `PriorityDecreaseOnUse` (=30), `PriorityRecoveryPerRound` (=10)
- `GetEffectivePriority()` = SkillPriority + EvaluateConditionBonus() (virtual, override point)
- `OnSkillUsed()`: decreases priority. Called by SkillComponent after Execute.
- `RecoverPriority()`: restores priority each round. Called in OnTurnStart.

Two-step enemy planning:
1. `AllocateEnemySkills()`: collects all AP-costing skills from all enemies, sorts by GetEffectivePriority() descending, greedily allocates faction AP (one skill per unit max). Stores in `TMap<ALFPTacticsUnit*, ULFPSkillBase*> AllocatedSkills`.
2. `ProcessNextEnemyPlan()`: iterates enemies by speed order, passes pre-allocated skill to `AIController->CreateActionPlan(PreAllocatedSkill)`.

`CreateActionPlan(ULFPSkillBase* PreAllocatedSkill = nullptr)`: uses pre-allocated skill if provided, otherwise falls back to `SelectBestSkill()` (which returns default attack if nothing else available).

## 5. Delegates
- `OnPhaseChanged(EBattlePhase)` — TurnManager broadcasts phase transitions
- `OnFactionAPChanged(EUnitAffiliation, int32)` — TurnManager broadcasts AP changes
- `OnTurnChanged` — existing turn change delegate
- `OnHealthChangedSignature`, `OnUnitDeathSignature` — unit events
- `OnSkillExecutedSignature` — SkillComponent event

## 6. UI Bindings
- `LFPSkillSelectionWidget`: `ActionPointsText` (BindWidgetOptional) shows "AP: X/Y", bound to OnFactionAPChanged
- `LFPTurnSpeedListWidget`: `PhaseText` (BindWidgetOptional) shows current phase name
- `LFPPlannedSkillIconWidget`: shows skill icon above enemy head during planning phase

## 7. Terrain Type System (completed)
Data-driven terrain with variable movement cost:
- `ELFPTerrainType` enum in `LFPBattleTypes.h`: Grass, Sand, Dirt, Stone, Snow, Water, Lava, Magic, Bridge
- `ULFPTerrainDataAsset` (header-only DataAsset) in `HexGrid/LFPTerrainDataAsset.h`: TerrainType, MovementCost (int32), bIsWalkable, DefaultSprite, DisplayName
- `ALFPHexTile`: added `TerrainData` property, `GetMovementCost()`, `SetTerrainData()` (syncs walkability + sprite), `DecorationSpriteComponent` (visual-only overlay)
- `ALFPHexGridManager`: `DefaultTerrainData` property applied in `GenerateGrid()`. `GetTilesInRange()` converted from BFS to Dijkstra (variable cost). `FindPath()` A* uses `GetMovementCost()`.
- `ALFPTacticsUnit`: `MoveToTile()` and `FinishMove()` compute actual path cost (sum of tile costs) instead of path length.
- Skill ranges unaffected (based on hex distance, not movement cost). AI auto-benefits via GridManager calls.

## 8. Data-Driven Map System + In-Game Map Editor (completed)
Map data stored as DataTable/CSV, runtime editor for map creation:
- `FLFPMapTileRow` (FTableRowBase) in `HexGrid/LFPMapData.h`: Q, R, TerrainType, DecorationID, SpawnFaction, SpawnIndex, EventTag. RowName = "Q_R".
- `ELFPSpawnFaction`: SF_None, SF_Player, SF_Enemy, SF_Neutral (separate from EUnitAffiliation to include None).
- `ALFPHexGridManager` registries: `TerrainRegistry` (ELFPTerrainType→DataAsset), `DecorationRegistry` (FName→Sprite), `MapDataTable`.
- Grid methods: `SpawnTileAt()`, `ClearGrid()`, `AddTile()`, `RemoveTile()`, `LoadMapFromDataTable()`, `LoadMapFromCSV()`, `SaveMapToCSV()`, `ExportMapData()`, `GetSpawnPoints()`.
- `GenerateGrid()` refactored to use SpawnTileAt. BeginPlay checks MapDataTable first.
- CSV format: UE DataTable compatible. Saved to `{ProjectSavedDir}/Maps/`. Loadable via temporary DataTable.
- `ULFPMapEditorComponent` on PlayerController: tool modes (Terrain/Decoration/SpawnPoint/Event/AddTile/RemoveTile), brush params, Save/Load/NewMap.
- `ULFPMapEditorWidget`: BindWidgetOptional pattern, tool buttons, terrain/decoration/faction dropdowns, file name input.
- PlayerController: ToggleEditorAction input, OnConfirmAction routes to editor when active, Z=0 plane raycast for AddTile.

## 9. World Map System (design phase)
关卡地图：节点-边网络，玩家从中心出发向四面八方探索。

### 游戏流程
- 单局 RPG 冒险，有主线目标（消灭最终 Boss）
- 回合压力机制：100 回合基准后全局难度递增（Boss 变强、瘟疫、极端气候）
- 移动消耗整数回合（边长度不等），路线规划是核心策略
- 可回头走已探索路径，已触发的战斗/事件节点不再触发

### 节点类型
- **战斗**：星级难度提示，首次触发。可撤退（损失资源，类型由敌人决定：野兽→食物，人类→金币，龙→贵重宝物/大量金币）。部分节点不可逃跑（有提示）
- **事件**：首次触发
- **商店**：购买道具、单位
- **城镇**：安全区，可反复进入。买卖/休息/接任务。有传送阵（部分需激活）用于城镇间快速穿梭
- **Boss**：最终目标
- **任务 NPC**：支线任务
- **技能节点**：获取技能点数，用于技能树提升

### 节点数据
- ID、世界坐标、节点类型、类型参数
- 战斗节点：关联战斗地图文件名、敌人星级、可否逃跑
- 城镇节点：商店配置、传送阵状态
- 事件节点：事件 ID
- 解锁条件（前置任务/节点），用于任务门控

### 边数据
- 起点 ID、终点 ID、移动回合消耗（整数）

### 迷雾系统
- 以玩家所在节点为圆心，按图距离（经过几条边）计算视野范围
- 视野内节点揭开后永久可见

### 场景管理
- **不同 Level 方案**：关卡地图是 Persistent Level，战斗场景是独立 Level
- 进入战斗时 Load Stream Level，隐藏关卡地图 Actor
- 战斗结束后卸载战斗 Level，恢复关卡地图
- 战斗地图数据量小（六角格 + Paper2D sprite），加载快
- 过场动画（地图卷起/展开）掩盖加载时间，强化"古老地图"氛围

### GameMode 架构（已重构）
- **世界地图模式**：`ALFPWorldMapGameMode` + `ALFPWorldMapPlayerController`
  - 世界地图管理器初始化
  - 节点交互（进入战斗/事件/商店）
  - 世界地图编辑器
  - 玩家移动和迷雾系统
- **战斗模式**：`ALFPTurnGameMode` + `ALFPTacticsPlayerController`
  - 回合管理器初始化
  - 单位选择/移动/技能释放
  - 战斗地图编辑器
  - 敌人 AI 和回合流程
- **职责分离**：不同 Level 使用不同 GameMode，PlayerController 职责清晰，无需状态标志区分模式

### 场景切换（方案 C: GameInstance + OpenLevel）
- `ULFPGameInstance`：跨关卡生命周期，持有世界地图快照、战斗请求/结果
- **世界地图 → 战斗**: `EnterBattle()` 保存 `FLFPWorldMapSnapshot` + 设置 `FLFPBattleRequest` → `OpenLevel(战斗关卡)`
- **战斗 → 世界地图**: `EndBattle()` 设置 `FLFPBattleResult` → `OpenLevel(世界地图关卡)`
- **世界地图恢复**: `WorldMapGameMode::StartPlay()` 检查快照 → 加载地图 → 恢复 PlayerState → 恢复已触发节点 → 处理战斗结果
- 数据结构: `FLFPBattleRequest`（节点ID/地图名/星级/可逃跑）、`FLFPBattleResult`（胜利/逃跑）、`FLFPWorldMapSnapshot`（节点ID/回合/访问集/揭露集/触发集）
- Consume 模式：读取后清除，防止重复处理

### 渲染方式
- 世界中 Actor 渲染（非 UI），微倾斜俯视角，"古老地图"视觉风格
- `ALFPWorldMapNode` Actor：节点，Sprite 表示类型，悬停显示信息
- `ALFPWorldMapEdge` Actor：边，Spline 或线段连接节点
- `ALFPWorldMapManager`：管理节点/边，加载/保存数据，迷雾逻辑

### 编辑器设计
- 运行时游戏内编辑器（和现有战斗地图编辑器一致）
- **嵌套编辑**：关卡地图编辑器（外层）点击战斗节点 → 切换到战斗地图编辑器（内层，已有）
- 外层：放置/删除/移动节点，连接/断开边，设置节点类型和参数
- 内层：编辑六角格地形、出生点等（已实现）
- 保存战斗地图后自动关联回该战斗节点，返回关卡地图编辑器

### 玩家成长途径
- **获得新单位**：主要通过战斗中背叛机制招降敌方单位（有次数限制，可通过技能/道具提高上限），也可城镇购买
- **道具系统**：遗物模式，全队生效（非单位装备）。通过战斗掉落、事件、支线任务、商店获得
- **单位升阶**：合并同种单位（2 个同阶同种 → 升一阶）。与背叛机制天然配合
- **学习新技能**：访问地图上的技能节点获取点数，在技能树中提升

## 10. 队伍与备战营系统 (completed)
玩家队伍管理：出战队伍 + 备战营，跨关卡持久化。

### 数据层
- `FLFPUnitEntry`（在 LFPGameInstance.h 中）：轻量单位数据，TypeID (FName) + Tier (int32)
- `ULFPUnitRegistryDataAsset`（Core/）：全局注册表，FName TypeID → {蓝图类, 显示名, 图标}
- `ALFPTacticsUnit`：新增 `UnitTypeID`、`UnitTier`、`ToUnitEntry()` 身份字段

### 队伍管理（GameInstance）
- `PartyUnits` / `ReserveUnits`：TArray<FLFPUnitEntry>
- `MaxPartySize = 3` / `MaxReserveSize = 3`（可配置）
- `TryAddUnit()`：先队伍 → 再备战 → 都满返回 false
- `ReplacePartyUnit()` / `ReplaceReserveUnit()` / `RemovePartyUnit()` / `RemoveReserveUnit()`
- 每场战斗 HP 恢复满，不持久化 HP

### 战斗捕获流程
- `ChangeAffiliation()`（敌→玩家）自动调用 `TurnGameMode->RecordCapturedUnit()`
- `ALFPTurnGameMode`：`CapturedUnits` 列表 + `RecordCapturedUnit()`
- `EndBattle()` 胜利时将 CapturedUnits 写入 `FLFPBattleResult.CapturedUnits`

### 战后处理（WorldMapGameMode）
- `HandleBattleResult()` → `ProcessCapturedUnits()`
- 逐个 `TryAddUnit()`，失败的进 `PendingCapturedUnits` 队列
- 队列非空时 `ShowNextReplacementUI()` → 创建 `ULFPUnitReplacementWidget`
- Widget 提供 `ReplacePartySlot()` / `ReplaceReserveSlot()` / `DiscardNewUnit()`
- 完成后通过 `OnReplacementComplete` 委托处理下一个

## 11. 战斗布置阶段 (completed)
战斗开始前玩家手动布置单位到出生点。

### 阶段枚举
- `EBattlePhase` 新增 `BP_Deployment`，作为战斗第一阶段

### TurnManager 流程变更
- `StartGame()`: 仅注册非玩家单位，设置 `CurrentPhase = BP_Deployment`（不广播，避免时序问题）
- `EndDeploymentPhase()`: 收集场上所有玩家单位注册 → `BeginNewRound()`

### 时序设计（重要）
- `GameMode::StartPlay()` 在 `PlayerController::BeginPlay()` 之前执行
- `StartGame()` 仅设置 CurrentPhase，不调用 SetPhase() 广播
- `PlayerController::BeginPlay()` 绑定委托后主动检查当前阶段，发现 BP_Deployment 则调用 `OnDeploymentPhaseStarted()`

### PlayerController 布置交互
- `OnDeploymentPhaseStarted()`: 高亮出生点、创建 DeploymentWidget
- `StartPlacingUnit(PartyIndex)`: 从注册表生成单位 Actor，跟随鼠标
- `PlaceUnit(Tile)`: 放到出生点格子
- `CancelPlacing()`: 销毁预览，退出放置模式
- `PickupDeployedUnit()`: 拾起已放置的重新放
- `ConfirmDeployment()`: 清除高亮 → TurnManager::EndDeploymentPhase()
- Tick 中 bIsPlacingUnit 时更新预览位置
- OnConfirmAction/OnCancelAction 中优先处理布置阶段

### DeploymentWidget
- BindWidget 模式：`Button_Unit0/1/2`、`Image_Unit0/1/2`、`Button_Confirm` 与蓝图绑定
- `Setup()` / `MarkUnitPlaced()` / `SetConfirmEnabled()` / `MarkUnitSelecting()`
- `OnUnitSelected` / `OnConfirmPressed` 委托
- NativeConstruct 中绑定按钮 OnClicked
- 已放置单位半透明显示（0.5 alpha）

## 12. TurnGameMode 生成管理器 (completed)
GameMode 负责生成 GridManager 和 TurnManager（而非预放置或场景查找）。

### 生成流程
- `StartPlay()`: 读取 BattleRequest → 生成 GridManager → CSV 加载地图 → 生成 TurnManager → StartGame()
- `GridManagerClass`（EditDefaultsOnly）：蓝图中配置，未配置则用默认 C++ 类
- GridManager/TurnManager 存为成员变量，提供 Getter
- CSV 路径：`{ProjectSavedDir}/Maps/{BattleMapName}.csv`
- GridManager::BeginPlay() 中原有的自动加载已注释掉，改为由 GameMode 控制

## 13. 战斗结束 + 掉落结算系统 (completed)
自动胜负判定、击杀掉落追踪、全屏结算面板。

### 胜负判定
- `TurnManager::CheckBattleEnd()`: 在 `UnregisterUnit()` 后调用，遍历存活单位
- 全敌方消灭 → 胜利，全玩家消灭 → 失败
- `bBattleEnded` 标志防重复触发，`PassTurn/BeginUnitTurn/BeginNewRound/EndCurrentRound` 加守卫

### 掉落系统
- `ALFPTacticsUnit`: `DropGold`/`DropFood`（蓝图配置）
- `HandleDeath()` 中敌方单位调用 `TurnGameMode::OnEnemyUnitKilled()` 累加掉落
- `FLFPBattleRequest`: 新增 `BaseGoldReward`/`BaseFoodReward`（节点基础奖励）
- `FLFPBattleResult`: 新增 `GoldReward`/`FoodReward`（总奖励 = 基础 + 击杀）

### 结算 UI
- `ULFPBattleResultWidget`（BindWidget 模式）：Text_Result/Text_Gold/Text_Food/Button_Confirm/Box_CapturedUnits
- `EndBattle()` 先显示结算面板 → 玩家确认 → `OnBattleResultConfirmed()` 写回结果并转场
- 输入模式切换：显示面板时 UIOnly，确认后恢复 GameAndUI

### 资源系统
- `ULFPGameInstance`: `Gold`/`Food` 持久化字段 + `AddGold()`/`AddFood()`
- `WorldMapGameMode::HandleBattleResult()`: 胜利时调用 AddGold/AddFood

## 15. 单位构建体系（进行中）
静态模板继续放在 `ULFPUnitRegistryDataAsset`，运行时状态继续放在 `ALFPTacticsUnit`，不新建独立 UnitDefinition 资产。

### 数据分层
- `FLFPUnitRegistryEntry` 扩展为：`Race`、`SpecialTags`、`BaseStats`、`AdvancedStats`
- `ALFPTacticsUnit` 增加：
  - 唯一种族：`ELFPUnitRace UnitRace`
  - 多标签：`FGameplayTagContainer SpecialTags`
  - 基础值：Attack / MaxHealth / MaxMovePoints / Speed + 高级属性基础值
  - 当前值：CurrentAttack / CurrentMaxHealth / CurrentMovePoints / CurrentSpeed + 当前高级属性
- 旧字段 `AttackPower` / `MaxHealth` / `MaxMovePoints` / `Speed` / `Defense` 仍保留兼容，但运行时逻辑已逐步桥接到当前值 getter

### 初始化链路
- `ALFPTacticsUnit::InitializeFromRegistry()`：按 `UnitTypeID` 从注册表灌入模板数据
- `ApplyRegistryEntry()`：写入种族、标签、基础属性、高级属性
- `ResetCurrentStatsToBase()`：将当前值重置为基础值
- `BeginPlay()` 中预放置单位会尝试自动从 `GameInstance->UnitRegistry` 初始化，避免只对部署生成生效

### 创建与遗物
- 玩家部署生成流程：`StartPlacingUnit()` 中在设置 `UnitTypeID/UnitTier` 后，先 `InitializeFromRegistry()`，再 `ApplyOwnedRelicsToUnit()`
- `ULFPGameInstance::ApplyOwnedRelicsToUnit()` 已落地：遗物只修改当前值，不改基础值

### 当前目标边界
- 本轮只完成“数据结构 + 运行时初始化/读取”
- 单位详情 UI（基础/高级属性显示切换）尚未开始做
