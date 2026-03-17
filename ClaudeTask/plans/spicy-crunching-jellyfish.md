# 单位移动动画

## Context
目前单位移动是瞬间传送（MoveToTile 直接调用 FinishMove）。需要改为逐格平滑移动动画，移动期间锁定玩家输入。代码中已有 FTimeline 相关基础设施但全部注释掉了，且依赖蓝图配置 MoveCurve。改用更简单的 Tick 驱动方式，无需额外蓝图配置。

## 修改文件

| 文件 | 操作 |
|------|------|
| `Source/LFP2D/Unit/LFPTacticsUnit.h` | 修改：加 bIsMoving、OnMoveFinished 委托、MoveSpeed |
| `Source/LFP2D/Unit/LFPTacticsUnit.cpp` | 修改：MoveToTile 启动动画、Tick 驱动逐格插值、FinishMove 广播委托 |
| `Source/LFP2D/Player/LFPTacticsPlayerController.h` | 修改：加 bWaitingForMove、OnUnitMoveComplete |
| `Source/LFP2D/Player/LFPTacticsPlayerController.cpp` | 修改：MoveUnit 绑定完成回调、游戏输入锁定（相机操作不锁） |
| `Source/LFP2D/Turn/LFPTurnManager.cpp` | 修改：AI 规划阶段移动后等待动画完成再推进 |

## 实现步骤

### Step 1: LFPTacticsUnit — 移动动画核心

**h 文件** 新增 `bIsMoving`、`MoveSpeed`、`OnMoveFinished` 委托。清理注释掉的 MoveTimeline 成员。

**MoveToTile()** — 不再直接调用 FinishMove，改为设 `bIsMoving = true` 启动动画状态。

**Tick()** — bIsMoving 时按 MoveSpeed 推进 MoveProgress，逐格 Lerp 位置，到达终点调 FinishMove。

**FinishMove()** — 设 bIsMoving = false，保留 SetCurrentCoordinates + ConsumeMovePoints，末尾 Broadcast OnMoveFinished。

### Step 2: PlayerController — 输入锁定

**MoveUnit()** — 调用 MoveToTile 后设 bWaitingForMove = true，绑定 OnMoveFinished → OnUnitMoveComplete。

**OnUnitMoveComplete()** — 设 bWaitingForMove = false，解绑，刷新范围显示。

**输入回调守卫** — OnSelectStarted/Completed、OnConfirmAction、OnCancelAction 等开头加 `if (bWaitingForMove) return;`。相机操作（Pan/Drag/Zoom）不锁。

### Step 3: TurnManager — AI 规划阶段等待移动

**ProcessNextEnemyPlan** 中 AI 调用 MoveToTile 后，如果 bIsMoving，绑定 OnMoveFinished 延迟推进下一个敌人规划（而非立即推进）。

## 验证
1. 玩家单位逐格平滑移动，期间不能操作（但可移动相机）
2. AI 规划阶段敌方单位逐格移动到施法位
3. MoveSpeed 可在蓝图中调节
