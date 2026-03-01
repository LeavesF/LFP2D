# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在本仓库工作时提供指导。

## 项目概述

LFP2D 是一款基于 Unreal Engine 5.5 + Paper2D 的 2D 回合制战棋 RPG。核心特性包括：六角网格移动、速度制回合系统、两阶段战斗流程（敌方规划 → 行动）、阵营共享 AP、技能优先级分配、AI 行为树、单位背叛系统。

## 构建 & 开发

Unreal Engine 5.5 C++ 项目，运行时模块为 **LFP2D**。

**命令行构建 (Windows):**
```
"<UE5_Install>/Engine/Build/BatchFiles/Build.bat" LFP2D Win64 Development "D:/UE_Projects/LFP2D/LFP2D.uproject" -waitmutex
```

**编辑器构建:**
```
"<UE5_Install>/Engine/Build/BatchFiles/Build.bat" LFPEditor Win64 Development "D:/UE_Projects/LFP2D/LFP2D.uproject" -waitmutex
```

**模块依赖** (LFP2D.Build.cs): Core, CoreUObject, Engine, InputCore, EnhancedInput, Paper2D, SlateCore, GameplayTags

**插件:** CommonUI (Runtime), ModelingToolsEditorMode (Editor only)

## 架构

### 核心系统 (Source/LFP2D/)

- **Core/** — `LFPTurnGameMode`: 游戏模式，启动时初始化回合系统。
- **Turn/** — `LFPTurnManager`: 速度制回合管理、回合周期、单位注册。管理两阶段战斗流程（EnemyPlanning → ActionPhase → RoundEnd）。管理阵营共享 AP 池。
  - `LFPBattleTypes.h`: 共享类型定义（`EBattlePhase`、`EUnitAffiliation`、`FEnemyActionPlan`）
- **Player/** — `LFPTacticsPlayerController`: 玩家控制器，处理 Enhanced Input、摄像机控制（平移/拖拽/缩放）、单位选择、移动指令、技能释放。管理游戏状态（MoveState, SkillReleaseState）。
- **HexGrid/** — `LFPHexGridManager` + `LFPHexTile`: 六角网格生成（立方体坐标 Q, R, S）、A* 寻路、范围计算、格子占用/可行走性、精灵图层高亮（移动/攻击/技能效果/路径）。
- **Unit/** — `LFPTacticsUnit`: 核心单位 Actor，属性（HP、攻击、防御、移动范围、速度）、阵营（Player/Enemy/Neutral）、Timeline 移动动画、PaperSprite 渲染。AP 方法代理到 TurnManager 的阵营 AP 系统。
- **Skill/** — 技能系统：
  - `LFPSkillBase`（抽象基类）：定义释放范围和效果范围的六角坐标模式，支持多种目标类型，有冷却和 AP 消耗。三层检查：`IsAvailable()` → `CanReleaseFrom()` → `CanExecute()`。技能优先级系统（SkillPriority，使用后降低，每回合恢复）。
  - `LFPSkillComponent`（单位技能管理器）
  - `LFPSkillDataAsset`（数据驱动技能定义）
  - `LFPSkillButtonWidget`（UI）
- **AI/** — `LFPAIController`：规划阶段方法（`CreateActionPlan`、`SelectBestSkill`、`FindBestCasterPosition`）。`LFPEnemyBehaviorData` 数据资产定义 AI 性格。BT 任务节点位于 `AI/BehaviorTree/`。
- **Unit/Betrayal/** — 背叛系统：`LFPBetrayalManager`、`LFPBetrayalCondition`（抽象）、`LFPBCondition_HPLack`（低血触发）。
- **UI/Fighting/** — 战斗 UI：`LFPSkillSelectionWidget`（技能面板，显示阵营 AP）、`LFPTurnSpeedListWidget`/`LFPTurnSpeedIconWidget`（回合顺序显示）、`LFPHealthBarWidget`、`LFPPlannedSkillIconWidget`（头顶技能图标）。

### 战斗流程

```
TurnManager（速度制回合）
  每回合开始:
    1. 敌方规划阶段 (EnemyPlanning)
       AllocateEnemySkills()  — 全局按技能优先级分配阵营 AP
       ProcessNextEnemyPlan() — 按速度顺序：选目标 → 移动到施法位 → 显示头顶图标
    2. 行动阶段 (ActionPhase)
       按速度排序轮流行动:
         玩家单位 → PlayerController 输入控制
         敌方单位 → 执行预规划的技能
    3. 回合结束 (RoundEnd)
       回复阵营 AP、重置单位状态
```

### 设计模式

- **组件化**: 单位挂载 SkillComponent、HealthBar WidgetComponent
- **事件驱动**: 多播委托（OnTurnChanged, OnPhaseChanged, OnFactionAPChanged, OnHealthChanged, OnSkillExecuted, OnDeath）
- **数据驱动**: 技能和 AI 行为通过 UDataAsset 子类和蓝图配置
- **行为树**: AI 决策使用自定义 BTTask 和 BTService 节点

### 内容组织

- `Content/Battle/Skill/Skills/` — 技能蓝图实现（如 BP_Skill_TestArrow, BP_Skill_TestRecover）
- `Content/Map/2DMap.umap` — 主战斗地图
- `Content/Core/BP_TacticsPC` — 玩家控制器蓝图
- `Content/AI/` — 行为树资产
- `Content/UI/Fighting/` — 战斗 UI Widget 蓝图
- `Content/Input/` — Enhanced Input 映射配置

## 代码规范

- 类前缀: `LFP`（如 `ALFPTacticsUnit`, `ULFPSkillBase`, `ALFPHexTile`）
- 头文件使用 `#pragma once`
- 使用 Enhanced Input System（非旧版输入）
- 技能定义为 `ULFPSkillBase` 的蓝图子类，`Execute()` 在蓝图中实现
- 六角坐标使用立方体坐标结构 `FLFPHexCoordinates`（Q, R, S 字段）
- 代码注释使用中文
- Git 提交格式: `[*]` 修改, `[+]` 新增, `[-]` 删除
