# 世界地图系统实现计划

## Context

LFP2D 已完成战斗地图系统（六角格、地形、编辑器、CSV 保存/加载）。下一步是实现世界地图系统——节点-边网络，玩家在节点间移动消耗回合，触发战斗/事件/商店等。设计方案已在 architecture.md 第 9 节记录。

本计划分 5 个阶段递增实现，每阶段可独立编译验证。

---

## Phase 1: 数据结构 + WorldMapManager（纯数据层）

**目标**: 定义所有核心类型，实现节点-边图的内存管理和 CSV 持久化。无可视化。

### 新建文件

| 文件 | 内容 |
|------|------|
| `Source/LFP2D/WorldMap/LFPWorldMapData.h` | `ELFPWorldNodeType` 枚举（7 种节点类型），`FLFPWorldNodeRow`（节点 CSV 行），`FLFPWorldEdgeRow`（边 CSV 行） |
| `Source/LFP2D/WorldMap/LFPWorldMapNode.h/.cpp` | 节点 Actor：持有 NodeID、NodeType、战斗/事件参数、运行时状态（bHasBeenTriggered、bIsRevealed）。`InitFromRowData()`、`ExportToRowData()`、`GetPosition2D()`、`SetPosition2D()` |
| `Source/LFP2D/WorldMap/LFPWorldMapEdge.h/.cpp` | 边 Actor：持有 FromNodeID、ToNodeID、TravelTurnCost。`InitFromRowData()`、`ExportToRowData()` |
| `Source/LFP2D/WorldMap/LFPWorldMapManager.h/.cpp` | Manager Actor：`TMap<int32, ALFPWorldMapNode*> NodeMap`、`TMap<FIntPoint, ALFPWorldMapEdge*> EdgeMap`、`TMap<int32, TSet<int32>> AdjacencyList`。方法：`SpawnNode()`、`RemoveNode()`、`AddEdge()`、`RemoveEdge()`、`GetNode()`、`GetNeighbors()`、`GetNextNodeID()`、`SaveWorldMap()`、`LoadWorldMap()` |

### 数据结构设计

**ELFPWorldNodeType**: `WNT_Battle`, `WNT_Event`, `WNT_Shop`, `WNT_Town`, `WNT_Boss`, `WNT_QuestNPC`, `WNT_SkillNode`

**FLFPWorldNodeRow** (FTableRowBase, RowName = NodeID 字符串):
- `int32 NodeID`, `float PosX`, `float PosY`, `ELFPWorldNodeType NodeType`
- 战斗参数: `FString BattleMapName`, `int32 StarRating`, `bool bCanEscape`
- 事件参数: `FString EventID`
- 解锁条件: `FString PrerequisiteNodeIDs`（分号分隔）

**FLFPWorldEdgeRow** (FTableRowBase, RowName = "FromID_ToID"):
- `int32 FromNodeID`, `int32 ToNodeID`, `int32 TravelTurnCost`

**CSV 存储**: 两个独立文件 `{Saved/WorldMaps}/{MapName}_nodes.csv` 和 `{MapName}_edges.csv`，遵循现有 UE DataTable CSV 格式。

### 关键实现细节
- `MakeEdgeKey(A, B)` → `FIntPoint(Min(A,B), Max(A,B))`，确保无向图查找
- CSV 保存/加载复用 `ALFPHexGridManager` 的模式：手动构造 CSV 字符串保存，临时 `UDataTable` + `CreateTableFromCSVString` 加载
- `RemoveNode()` 自动移除所有关联边

### 不修改现有文件

### 验证方式
在测试关卡放置 `ALFPWorldMapManager`，蓝图中调用 `SpawnNode` / `AddEdge` / `SaveWorldMap` / `LoadWorldMap`，检查 CSV 文件生成和数据往返正确。

---

## Phase 2: 节点 & 边可视化（Paper2D 精灵渲染）

**目标**: 节点显示类型精灵，边绘制连接线。

### 新建文件

| 文件 | 内容 |
|------|------|
| `Source/LFP2D/WorldMap/LFPWorldNodeDataAsset.h` | 节点类型视觉数据资产：`DefaultSprite`、`TriggeredSprite`、`FogSprite`、`DisplayName` |

### 修改文件

| 文件 | 变更 |
|------|------|
| `LFPWorldMapNode.h/.cpp` | 添加 `UPaperSpriteComponent`（节点精灵）、`HighlightSpriteComponent`（选中高亮）、`NodeVisualData` 引用、`SetNodeVisualData()`、`UpdateVisualState()`、`SetHighlighted()` |
| `LFPWorldMapEdge.h/.cpp` | 添加 `UPaperSpriteComponent`（拉伸线段精灵）、`UpdateVisualPosition(Start, End)` 计算中点/旋转/缩放 |
| `LFPWorldMapManager.h/.cpp` | 添加 `TMap<ELFPWorldNodeType, ULFPWorldNodeDataAsset*> NodeVisualRegistry`，`SpawnNode` 时自动应用视觉数据 |

### 验证方式
放置 Manager，配置 NodeVisualRegistry，生成节点和边后在视口中看到精灵图标和连接线。

---

## Phase 3: 世界地图编辑器（运行时）

**目标**: 游戏内编辑器，支持放置/删除/移动节点、连接/断开边、设置节点参数、嵌套编辑战斗地图。

### 新建文件

| 文件 | 内容 |
|------|------|
| `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h/.cpp` | 编辑器组件（挂载在 PlayerController）。工具枚举 `ELFPWorldMapEditorTool`（PlaceNode/RemoveNode/MoveNode/ConnectEdge/RemoveEdge/SetNodeParams/EditBattleMap）。笔刷参数。`SelectNode()`、`PlaceNodeAt()`、`HandleEdgeConnection()`（两次点击连边）、`EnterBattleMapEditor()`、`ReturnFromBattleMapEditor()` |
| `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h/.cpp` | 编辑器 UI Widget。BindWidgetOptional 模式：工具按钮、节点类型下拉框、参数输入框、保存/加载按钮、当前工具/选中节点信息文本 |

### 修改文件

| 文件 | 变更 |
|------|------|
| `LFPTacticsPlayerController.h/.cpp` | 添加 `WorldMapEditorComponent`（构造函数创建）、`WorldMapEditorWidgetClass`/`WorldMapEditorWidget`、`ToggleWorldMapEditorAction` 输入绑定。`OnConfirmAction` 中添加世界地图编辑器输入路由（优先级：世界编辑器 > 战斗编辑器 > 游戏逻辑） |

### 嵌套编辑流程
1. 世界编辑器选中战斗节点 → `EditBattleMap` 工具
2. `EnterBattleMapEditor()`: 读取节点 `BattleMapName`，激活现有 `MapEditorComponent`，加载对应 CSV，显示战斗编辑器 UI
3. 战斗编辑完成 → `ReturnFromBattleMapEditor()`: 保存 CSV（文件名自动关联回节点），恢复世界编辑器 UI

### 验证方式
运行游戏，按键切换编辑器，放置/连接/移动节点，保存加载世界地图，嵌套编辑战斗地图。

---

## Phase 4: 玩家移动 + 迷雾系统

**目标**: 玩家在节点间移动消耗回合，迷雾按图距离揭开。

### 新建文件

| 文件 | 内容 |
|------|------|
| `Source/LFP2D/WorldMap/LFPWorldMapPlayerState.h/.cpp` | 玩家世界地图状态：`CurrentNodeID`、`CurrentTurn`、`TurnPressureThreshold`、`VisitedNodeIDs`、`RevealedNodeIDs`。`MoveToNode()`（验证边存在、消耗回合、更新位置）、`GetReachableNodeIDs()`（过滤解锁条件）。委托 `OnTurnChanged`、`OnPlayerNodeChanged` |

### 修改文件

| 文件 | 变更 |
|------|------|
| `LFPWorldMapManager.h/.cpp` | 添加 `FogVisionRange`(int32)、`PlayerState`(UObject)、`UpdateFog()` 方法、`GetNodesWithinGraphDistance()` BFS。`UpdateFog` 根据玩家位置 BFS 揭开节点，更新 `bIsRevealed` + `UpdateVisualState()` |
| `LFPTacticsPlayerController.h/.cpp` | 添加 `bIsOnWorldMap` 状态。非编辑模式下点击相邻节点 → 高亮可达节点 → 确认移动 → 触发节点事件 |

### 验证方式
在世界地图上移动玩家 token，观察回合消耗、迷雾揭开、节点首次触发标记。

---

## Phase 5: 场景切换（Level Streaming）

**目标**: 战斗地图作为流关卡加载/卸载，世界地图 Actor 在战斗时隐藏。

### 新建文件

| 文件 | 内容 |
|------|------|
| `Source/LFP2D/WorldMap/LFPSceneTransitionManager.h/.cpp` | `ELFPSceneState`（WorldMap/Transitioning/Battle）。`EnterBattle(BattleNode)`: 隐藏世界地图 Actor → 加载流关卡 → 战斗 HexGridManager 加载对应 CSV。`LeaveBattle(bVictory)`: 卸载流关卡 → 显示世界地图 → 更新节点状态。过场动画通过 `BlueprintImplementableEvent` 在蓝图中实现 |

### 修改文件

| 文件 | 变更 |
|------|------|
| `LFPTurnGameMode.h/.cpp` | `StartPlay()` 中根据场景类型条件性生成 TurnManager 或 WorldMapManager |
| `LFPTacticsPlayerController.h/.cpp` | 添加 `CachedSceneState`，输入路由根据场景状态分发到世界地图或战斗系统 |

### 验证方式
点击战斗节点 → 过场 → 加载战斗关卡 → 战斗结束 → 返回世界地图，节点标记为已触发。

---

## 实现顺序建议

先做 Phase 1，验证数据层正确后再逐步叠加。每个 Phase 完成后可编译运行验证，不破坏现有战斗系统。

Phase 1 和 Phase 2 可以合并实现（数据 + 可视化一起做），因为 Node/Edge Actor 从一开始就需要精灵组件。

## 参考文件
- 数据结构模式: [LFPMapData.h](Source/LFP2D/HexGrid/LFPMapData.h)
- CSV 保存/加载模式: [LFPHexGridManager.cpp](Source/LFP2D/HexGrid/LFPHexGridManager.cpp)
- 编辑器组件模式: [LFPMapEditorComponent.h](Source/LFP2D/HexGrid/LFPMapEditorComponent.h)
- 编辑器 Widget 模式: [LFPMapEditorWidget.h](Source/LFP2D/UI/MapEditor/LFPMapEditorWidget.h)
- PlayerController 输入路由: [LFPTacticsPlayerController.cpp](Source/LFP2D/Player/LFPTacticsPlayerController.cpp)
- DataAsset 模式: [LFPTerrainDataAsset.h](Source/LFP2D/HexGrid/LFPTerrainDataAsset.h)
