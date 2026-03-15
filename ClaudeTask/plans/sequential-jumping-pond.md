# 战斗布置阶段实现计划

## Context
战斗开始前需要一个布置阶段：显示玩家队伍单位列表，玩家将单位逐个放置到出生点格子上，全部放置完毕后按下确认按钮正式开始战斗。

## 现有系统可复用
- `EBattlePhase` 枚举（LFPBattleTypes.h）— 新增 BP_Deployment
- `ALFPHexGridManager::GetSpawnPoints(SF_Player)` — 获取玩家出生点格子
- `ALFPHexTile::SpawnFaction` / `SpawnIndex` — 格子上的出生点数据
- `ULFPGameInstance::PartyUnits` + `UnitRegistry` — 队伍数据和蓝图类映射
- `OnPhaseChanged` 委托 — 阶段切换通知
- 格子高亮层 — 复用来高亮出生点
- `ALFPTacticsUnit::SetCurrentCoordinates()` — 放置单位到格子

## 方案

### 1. `EBattlePhase` 新增 `BP_Deployment`
**文件**: `Source/LFP2D/Turn/LFPBattleTypes.h`

在枚举最前面添加 `BP_Deployment`，作为战斗第一阶段。

### 2. `ALFPTurnManager` 布置阶段流程
**文件**: `Source/LFP2D/Turn/LFPTurnManager.h/.cpp`

修改 `StartGame()`：
- 原来：收集所有单位 → 0.5s 后 BeginNewRound()
- 改为：仅收集敌方单位并注册 → `BeginDeploymentPhase()`

新增方法：
- `BeginDeploymentPhase()` — SetPhase(BP_Deployment)
- `EndDeploymentPhase()` — 注册所有已布置的玩家单位 → BeginNewRound()

### 3. `ALFPTacticsPlayerController` 布置交互
**文件**: `Source/LFP2D/Player/LFPTacticsPlayerController.h/.cpp`

新增成员：
- `bool bIsInDeployment` — 是否处于布置阶段
- `bool bIsPlacingUnit` — 是否正在放置单位（跟随鼠标）
- `int32 PlacingPartyIndex = -1` — 当前放置的队伍索引
- `ALFPTacticsUnit* PlacingUnitPreview` — 跟随鼠标的单位 Actor
- `TArray<ALFPTacticsUnit*> DeployedUnits` — 已布置的单位列表
- `TArray<ALFPHexTile*> PlayerSpawnTiles` — 缓存出生点
- `TSubclassOf<ULFPDeploymentWidget> DeploymentWidgetClass` — 布置 UI 类
- `ULFPDeploymentWidget* DeploymentWidget` — 布置 UI 实例

布置阶段交互：
- **Tick（布置中）**: `bIsPlacingUnit` 时射线投射 Z=0 平面更新预览单位位置
- **OnConfirmAction（布置中）**:
  - 正在放置：射线检测格子 → 空闲玩家出生点 → `PlaceUnit()`
  - 不在放置：点击已放置的单位 → `PickupDeployedUnit()`
- **OnCancelAction（布置中）**: 取消当前放置 → `CancelPlacing()`

新增方法：
- `OnDeploymentPhaseStarted()` — 高亮出生点、创建 UI、绑定委托
- `StartPlacingUnit(int32 PartyIndex)` — 生成预览单位，进入放置模式
- `PlaceUnit(ALFPHexTile* Tile)` — 放置到格子，更新 UI
- `CancelPlacing()` — 销毁预览，退出放置模式
- `PickupDeployedUnit(ALFPTacticsUnit* Unit)` — 拾起已放置单位重新放置
- `ConfirmDeployment()` — 通知 TurnManager::EndDeploymentPhase()

### 4. `ULFPDeploymentWidget` 布置 UI
**新建**: `Source/LFP2D/UI/Fighting/LFPDeploymentWidget.h/.cpp`

C++ 骨架，蓝图实现布局（BindWidgetOptional 模式）：
- `Setup(PartyUnits, Registry)` — 初始化单位图标列表
- `MarkUnitPlaced(int32 PartyIndex, bool bPlaced)` — 标记已放置/未放置
- `SetConfirmEnabled(bool bEnabled)` — 全部放置后启用确认按钮
- `FOnDeploymentUnitSelected OnUnitSelected` — 委托(int32 PartyIndex)
- `FOnDeploymentConfirmed OnConfirmPressed` — 确认委托

## 战斗流程（改后）
```
StartGame()
  → 注册敌方单位（预放置在场上的）
  → BeginDeploymentPhase()  [新增]
    → OnPhaseChanged(BP_Deployment)
    → PlayerController 高亮出生点 + 显示布置 UI

[玩家布置循环]
  → 点击 UI 单位图标 → StartPlacingUnit(index)
    → 生成单位 Actor，跟随鼠标
  → 左键点击出生点 → PlaceUnit(tile)
  → 右键取消 → CancelPlacing()
  → 点击已放置单位 → PickupDeployedUnit()

全部放置 → 确认按钮 → ConfirmDeployment()
  → TurnManager::EndDeploymentPhase()
    → 注册所有玩家单位
    → BeginNewRound()（正常战斗流程）
```

## 涉及文件
| 文件 | 操作 | 内容 |
|------|------|------|
| `Turn/LFPBattleTypes.h` | 修改 | EBattlePhase 添加 BP_Deployment |
| `Turn/LFPTurnManager.h` | 修改 | 添加 BeginDeploymentPhase/EndDeploymentPhase |
| `Turn/LFPTurnManager.cpp` | 修改 | 实现布置阶段，修改 StartGame 流程 |
| `Player/LFPTacticsPlayerController.h` | 修改 | 添加布置相关状态和方法 |
| `Player/LFPTacticsPlayerController.cpp` | 修改 | 实现布置交互逻辑 |
| `UI/Fighting/LFPDeploymentWidget.h` | 新建 | 布置 UI Widget |
| `UI/Fighting/LFPDeploymentWidget.cpp` | 新建 | Setup/MarkPlaced/Confirm |

## 验证
1. 编译通过
2. 战斗地图 CSV 中配置 SF_Player 出生点格子
3. GameInstance 配置 PartyUnits 和 UnitRegistry
4. 进入战斗 → 布置阶段：出生点高亮，UI 显示队伍单位图标
5. 点击图标 → 单位跟随鼠标 → 左键点击出生点放置 → 右键取消
6. 可拾起已放置的重新放置
7. 全部放置后确认按钮亮起 → 点击确认 → 进入正常战斗流程
