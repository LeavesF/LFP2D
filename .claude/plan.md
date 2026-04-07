# 传送阵功能实现计划

## 需求
城镇内传送阵：只能从有传送阵的城镇传送到其他有传送阵的城镇，传送后自动进入目标城镇。

## 设计思路
目标节点**自动发现**：运行时扫描所有城镇节点，筛选 `TownBuildingList` 包含 "Teleport" 的节点作为可选目的地（排除当前所在节点）。不需要手动配置 `TeleportTargetNodeIDs`，简化编辑器工作。

## 需要修改/新增的文件

### 1. 新增 `LFPTeleportWidget.h/.cpp`（传送阵 UI）
- 轻量列表 Widget，显示可传送的城镇节点列表
- `Setup(TArray<ALFPWorldMapNode*> Targets)` 接收目标列表
- 每个条目显示节点 ID / 城镇名
- 点击条目 → 广播 `OnTeleportTargetSelected(int32 TargetNodeID)`
- 关闭按钮 → 广播 `OnClosed`

### 2. 修改 `LFPWorldMapManager.h/.cpp`
- 新增 `TeleportPlayerToNode(int32 TargetNodeID)`：
  - 直接设置 `PlayerState->CurrentNodeID`（不校验相邻、不消耗回合）
  - 更新 `VisitedNodeIDs`
  - 立即移动 Pawn 到目标位置（`SetLocationImmediate`，不走移动动画）
  - 调用 `UpdateFog()`
  - 广播 `OnPlayerNodeChanged`（如有）
- 新增 `GetTeleportDestinations(int32 ExcludeNodeID)` → 返回所有含传送阵的城镇节点（排除当前节点）

### 3. 修改 `LFPWorldMapPlayerState.h/.cpp`
- 新增 `SetCurrentNodeDirectly(int32 NodeID)` — 直接设置当前节点，不做相邻校验

### 4. 修改 `LFPWorldMapPlayerController.h/.cpp`
- 新增成员：`TeleportWidgetClass`、`TeleportWidget` 指针
- 新增 `OpenTeleport()` — 收集目标列表，创建/显示 TeleportWidget
- 新增 `OnTeleportTargetSelected(int32 TargetNodeID)` — 关闭所有 UI → Manager->TeleportPlayerToNode() → OpenTown(目标节点)
- 新增 `OnTeleportWidgetClosed()` ��� 返回城镇面板
- 填充 `OnTownBuildingRequested` 的 `TBT_Teleport` 分支

### 5. 蓝图配合
- 新建 `WBP_Teleport` UMG 蓝图（继承 LFPTeleportWidget），配置列表布局
- `BP_WorldMapPC` 中配置 `TeleportWidgetClass`

## 执行流程
```
玩家进入城镇 → 点击传送阵按钮
→ TownWidget 广播 TBT_Teleport
→ PlayerController::OnTownBuildingRequested
  → 隐藏 TownWidget
  → Manager->GetTeleportDestinations(当前节点ID) 获取目标列表
  → OpenTeleport(目标列表)
→ 玩家选择目标城镇
  → Manager->TeleportPlayerToNode(目标ID)
  → 关闭 TeleportWidget
  → OpenTown(目标城镇节点) ← 自动进入
```
