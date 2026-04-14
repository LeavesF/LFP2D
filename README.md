# LFP2D

LFP2D 是一个基于 Unreal Engine 5.5 的 2D 战棋 RPG 原型项目，核心结构是：

- 世界地图探索
- 进入战斗地图
- 六边形回合战斗
- 结算后返回世界地图并保留状态

项目只有一个运行时模块 `LFP2D`。主要玩法逻辑在 C++，可玩内容依赖 Blueprint、UMG、DataAsset 和 CSV。

这份 README 的目标不是讲全，而是让新对话能最快进入项目上下文。

## 项目概览

- 引擎版本：UE 5.5
- 主模块：`Source/LFP2D`
- Target：`LFP2DTarget`、`LFP2DEditorTarget`
- 主要依赖：`EnhancedInput`、`Paper2D`、`GameplayTags`
- 已启用插件：`CommonUI`、`ModelingToolsEditorMode`（仅 Editor）

## 建议先读

按这个顺序最快：

1. `README.md`
2. `ClaudeTask/memory/architecture.md`
3. `ClaudeTask/memory/progress.md`
4. `Source/LFP2D/Core/LFPGameInstance.h`
5. `Source/LFP2D/WorldMap/LFPWorldMapGameMode.h`
6. `Source/LFP2D/Core/LFPTurnGameMode.h`
7. `Source/LFP2D/Turn/LFPTurnManager.h`
8. `Source/LFP2D/Player/LFPTacticsPlayerController.h`

## 运行入口

- 编辑器启动地图：`Content/Map/Test_WorldMap.umap`
- 主菜单地图：`Content/Map/MainMenu.umap`
- 战斗测试地图：`Content/Map/Test_Fight.umap`
- 默认全局 GameMode：`BP_WorldMapGameMode`
- GameInstance：`BP_GameInstance`

注意：

`Config/DefaultEngine.ini` 里 `GameDefaultMap` 仍然是 UE 模板图，不是 `Test_WorldMap`。也就是说，编辑器打开时会进世界地图，但运行时默认入口配置并不完全一致，后续如果做打包或正式启动流程，需要先统一这里。

## 核心架构

### 世界地图层

主要文件：

- `Source/LFP2D/WorldMap/LFPWorldMapGameMode.*`
- `Source/LFP2D/WorldMap/LFPWorldMapManager.*`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.*`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerState.*`
- `Source/LFP2D/WorldMap/LFPWorldMapPawn.*`
- `Source/LFP2D/WorldMap/LFPWorldMapData.h`

职责：

- 管理节点/边世界地图
- 从 CSV 加载和保存地图
- 处理迷雾、移动、节点触发、回合压力
- 进入战斗、城镇、商店、雇佣市场、传送、升阶等节点功能

### 战斗层

主要文件：

- `Source/LFP2D/Core/LFPTurnGameMode.*`
- `Source/LFP2D/Turn/LFPTurnManager.*`
- `Source/LFP2D/Player/LFPTacticsPlayerController.*`
- `Source/LFP2D/HexGrid/LFPHexGridManager.*`
- `Source/LFP2D/HexGrid/LFPHexTile.*`
- `Source/LFP2D/Turn/LFPBattleTypes.h`

职责：

- 生成六边形地图和回合管理器
- 从 `Saved/Maps/` 加载战斗地图 CSV
- 管理战斗阶段：部署 -> 敌人规划 -> 行动 -> 回合结束
- 管理阵营共享 AP
- 结算战斗并把结果写回 `LFPGameInstance`

### 单位 / 技能 / AI

主要文件：

- `Source/LFP2D/Unit/LFPTacticsUnit.*`
- `Source/LFP2D/Core/LFPUnitRegistryDataAsset.*`
- `Source/LFP2D/Skill/LFPSkillBase.*`
- `Source/LFP2D/Skill/LFPSkillComponent.*`
- `Source/LFP2D/AI/LFPAIController.*`
- `Source/LFP2D/AI/BehaviorTree/*`
- `Source/LFP2D/Unit/Betrayal/*`

职责：

- 单位运行时属性、行动、归属、移动、死亡、捕获
- 通过 `UnitRegistry` 做单位模板初始化
- 通过技能系统和 Blueprint 子类驱动技能效果
- 敌方按技能优先级和阵营 AP 做规划

### 持久化 / 经济 / 队伍

主要文件：

- `Source/LFP2D/Core/LFPGameInstance.*`
- `Source/LFP2D/Shop/LFPRelicTypes.h`
- `Source/LFP2D/Shop/LFPRelicDataAsset.*`
- `Source/LFP2D/Shop/LFPShopDataAsset.*`
- `Source/LFP2D/Shop/LFPHireMarketDataAsset.*`

职责：

- 关卡切换时保存上下文
- 保存战斗请求、战斗结果、世界地图快照
- 保存读档
- 持久化金币、食物、遗物、队伍、备战队、雇佣市场购买记录

## 主流程

1. 世界地图在 `ALFPWorldMapGameMode` 中启动。
2. `ALFPWorldMapManager` 从 `Saved/WorldMaps/` 读取节点和边数据。
3. 玩家通过 `ALFPWorldMapPlayerController` 在节点间移动。
4. 进入战斗节点时，把 `FLFPBattleRequest` 写入 `ULFPGameInstance`，再切到战斗关卡。
5. `ALFPTurnGameMode` 读取请求，从 `Saved/Maps/` 加载战斗地图，生成管理器并开始战斗状态机。
6. 战斗结算确认后，把 `FLFPBattleResult` 写回 `ULFPGameInstance`，再返回世界地图。
7. `ALFPWorldMapGameMode` 恢复世界地图状态并处理奖励、捕获单位等后续逻辑。

## 目录重点

### 代码

`Source/LFP2D` 里最重要的目录：

- `Core/`：GameInstance、TurnGameMode、单位注册表
- `Turn/`：回合管理和共享战斗类型
- `WorldMap/`：世界地图运行逻辑
- `HexGrid/`：六边形地图、地形、地图编辑
- `Unit/`：单位和背叛系统
- `Skill/`：技能系统
- `AI/`：AI 控制器和行为树任务
- `UI/`：战斗 UI、世界地图 UI、菜单、编辑器 UI

### 资源

- `Content/Core/`：核心 Blueprint 类
- `Content/Map/`：关卡地图
- `Content/UI/`：UMG 界面
- `Content/Input/`：Enhanced Input 资源
- `Content/WorldMap/`：世界地图相关资源
- `Content/Battle/Skill/`：技能蓝图
- `Content/Unit/`：单位蓝图和注册表
- `Content/Grid/`、`Content/Art/HexGrid/`：地形和格子表现

### CSV 数据

项目把可编辑地图数据直接保留在仓库里：

- `Saved/Maps/*.csv`：战斗地图
- `Saved/WorldMaps/*_nodes.csv`：世界地图节点
- `Saved/WorldMaps/*_edges.csv`：世界地图边

当前示例数据：

- `Saved/Maps/test.csv`
- `Saved/WorldMaps/testworld_nodes.csv`
- `Saved/WorldMaps/testworld_edges.csv`

## 已落地的编辑器 / UI

- 战斗地图运行时编辑器
- 世界地图运行时编辑器
- 战斗部署 UI
- 战斗结算 UI
- 世界地图系统菜单
- 主菜单
- 城镇 / 商店 / 雇佣市场 / 传送 / 单位合并 UI

## 构建

Windows 示例：

```bat
"D:/UE/UE_5.5/Engine/Build/BatchFiles/Build.bat" LFP2D Win64 Development "D:/UE_Projects/LFP2D/LFP2D.uproject" -waitmutex
"D:/UE/UE_5.5/Engine/Build/BatchFiles/Build.bat" LFP2DEditor Win64 Development "D:/UE_Projects/LFP2D/LFP2D.uproject" -waitmutex
```

`LFP2D.Build.cs` 依赖：

- Public：`Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`Paper2D`、`SlateCore`、`GameplayTags`
- Private：`Json`、`JsonUtilities`

## 项目记忆文件

这些文件建议持续维护：

- `ClaudeTask/memory/MEMORY.md`：长期有效的约定、偏好、关键路径
- `ClaudeTask/memory/architecture.md`：系统设计和架构决策
- `ClaudeTask/memory/progress.md`：当前进度、已完成内容、已知问题
- `CLAUDE.md`：项目协作和构建背景

## 读代码时的注意点

- 这个项目非常依赖 Blueprint 绑定，单看 C++ 不一定能看出完整行为。
- 很多系统是 CSV 驱动的，逻辑不对时要同时看代码和 `Saved/Maps` / `Saved/WorldMaps`。
- 跨关卡状态几乎都经过 `ULFPGameInstance`，场景切换问题优先查这里。
- `UI/` 文件数量最多，但真正的主流程核心在 `Core`、`Turn`、`WorldMap`、`HexGrid`、`Unit`、`Skill`、`AI`。

## 按任务继续读

- 看战斗：`LFPTurnGameMode` -> `LFPTurnManager` -> `LFPTacticsPlayerController` -> `LFPTacticsUnit`
- 看世界地图：`LFPWorldMapGameMode` -> `LFPWorldMapManager` -> `LFPWorldMapPlayerController` -> `LFPGameInstance`
- 看数据驱动：`LFPMapData.h`、`LFPWorldMapData.h`、`LFPUnitRegistryDataAsset.*`、商店/遗物/雇佣市场数据资产
- 看历史上下文：`ClaudeTask/memory/architecture.md`、`ClaudeTask/memory/progress.md`
