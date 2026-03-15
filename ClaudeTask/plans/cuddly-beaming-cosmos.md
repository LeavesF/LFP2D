# 数据驱动地图生成 + 游戏内地图编辑器

## Context
当前 GridManager 只能生成固定矩形网格，无法加载预设地图数据。需要实现：
1. 数据驱动的地图加载（DataTable + CSV 格式）
2. 游戏内地图编辑器（运行时编辑、保存/加载）
3. 支持任意形状地图
4. 存储地形、装饰、出生点、事件触发区域

## 分阶段实施

### 第一阶段：数据结构 + 地图加载/保存

**新建文件：**
- `Source/LFP2D/HexGrid/LFPMapData.h` — 地图数据结构

**修改文件：**
- `Source/LFP2D/HexGrid/LFPHexTile.h/.cpp` — 添加出生点/事件/装饰运行时字段
- `Source/LFP2D/HexGrid/LFPHexGridManager.h/.cpp` — 地形注册表、地图加载/保存、SpawnTileAt、AddTile/RemoveTile

#### 1.1 LFPMapData.h（新建）

```cpp
// ELFPSpawnFaction 枚举：SF_None, SF_Player, SF_Enemy, SF_Neutral
// FLFPMapTileRow : FTableRowBase
//   - Q, R (int32)
//   - TerrainType (ELFPTerrainType)    — 存枚举值，加载时映射到 DataAsset
//   - DecorationID (FName)             — 装饰注册表 Key，NAME_None = 无
//   - SpawnFaction (ELFPSpawnFaction)   — 出生点阵营
//   - SpawnIndex (int32)               — 阵营内出生顺序，0 = 非出生点
//   - EventTag (FGameplayTag)          — 事件标签，空 = 无
// RowName 格式: "Q_R"（如 "3_-2"）
```

#### 1.2 LFPHexTile 新增字段

```cpp
// 出生点信息
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
ELFPSpawnFaction SpawnFaction = ELFPSpawnFaction::SF_None;
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
int32 SpawnIndex = 0;
// 事件标签
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
FGameplayTag EventTag;
// 装饰 ID
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
FName DecorationID = NAME_None;
// 运行时设置装饰精灵
void SetDecorationByID(FName InID, UPaperSprite* InSprite);
```

#### 1.3 LFPHexGridManager 重构

**新增属性：**
```cpp
// 地形注册表：枚举 → DataAsset（在蓝图中配置）
TMap<ELFPTerrainType, ULFPTerrainDataAsset*> TerrainRegistry;
// 装饰注册表：ID → 精灵
TMap<FName, UPaperSprite*> DecorationRegistry;
// 预设地图数据表
UDataTable* MapDataTable;
// HexSize/VerticalScale 公共 getter
```

**新增方法：**
- `ClearGrid()` — 销毁所有格子
- `SpawnTileAt(Q, R, TerrainType, DecorationID, SpawnFaction, SpawnIndex, EventTag)` — 生成单个格子，从注册表查找资产
- `AddTile(Q, R)` — 添加默认格子
- `RemoveTile(Q, R)` — 移除格子
- `LoadMapFromDataTable(UDataTable*)` — 清空 + 遍历行 + SpawnTileAt
- `LoadMapFromCSV(FString FilePath)` — 读文件 → 临时 DataTable → LoadMapFromDataTable
- `SaveMapToCSV(FString FilePath)` — 遍历 GridMap → 构建 CSV → FFileHelper 写入
- `ExportMapData()` — 返回 TArray<FLFPMapTileRow>
- `GetSpawnPoints(ELFPSpawnFaction)` — 获取指定阵营出生点
- `GetTerrainDataForType(ELFPTerrainType)` — 从注册表查找

**重构 GenerateGrid：** 内部循环改用 SpawnTileAt()
**重构 BeginPlay：** 有 MapDataTable 则 LoadMapFromDataTable，否则 GenerateGrid

**CSV 格式：**
```
---,Q,R,TerrainType,DecorationID,SpawnFaction,SpawnIndex,EventTag
0_0,0,0,TT_Grass,,SF_None,0,
1_0,1,0,TT_Sand,,SF_None,0,
0_1,0,1,TT_Grass,Tree_01,SF_Player,1,
2_-1,2,-1,TT_Stone,,SF_Enemy,1,Battle.Event.Ambush
```

---

### 第二阶段：游戏内地图编辑器

**新建文件：**
- `Source/LFP2D/HexGrid/LFPMapEditorComponent.h/.cpp` — 编辑器状态管理组件
- `Source/LFP2D/UI/MapEditor/LFPMapEditorWidget.h/.cpp` — 编辑器 UI

**修改文件：**
- `Source/LFP2D/Player/LFPTacticsPlayerController.h/.cpp` — 编辑模式输入路由

#### 2.1 LFPMapEditorComponent（新建）

挂载在 PlayerController 上的 ActorComponent。

**编辑器工具模式枚举：**
```
MET_None, MET_Terrain, MET_Decoration, MET_SpawnPoint, MET_Event, MET_AddTile, MET_RemoveTile
```

**核心方法：**
- `ToggleEditorMode()` — 开关编辑器
- `SetCurrentTool(ELFPMapEditorTool)` — 选择工具
- 笔刷参数 Setter：地形类型、装饰 ID、出生点阵营/序号、事件标签
- `ApplyToolToTile(ALFPHexTile*)` — 对已有格子应用工具
- `ApplyToolToCoord(Q, R)` — 对空位应用（添加格子）
- `SaveMap(FileName)` / `LoadMap(FileName)` — 代理到 GridManager
- `NewMap(Width, Height)` — 创建空白地图
- `GetSavedMapList()` — 列出已保存地图

#### 2.2 PlayerController 集成

- 构造函数创建 MapEditorComponent
- 新增 ToggleEditor 输入绑定
- `OnConfirmAction()` 中：编辑器激活时路由到编辑器组件
- AddTile 工具检测空白区域坐标：射线投射到 Z=0 平面 + FromWorldLocation 转换

#### 2.3 LFPMapEditorWidget（新建）

UUserWidget + BindWidget 模式（同 SkillSelectionWidget）：
- 工具按钮×6（地形/装饰/出生点/事件/添加/移除）
- 地形类型下拉框 (ComboBoxString)
- 装饰下拉框
- 出生点阵营下拉框 + 序号输入
- 文件名输入 + 保存/加载/新建按钮
- 当前工具指示文本

对应需要创建 UMG 蓝图 `Content/UI/MapEditor/WBP_MapEditor`。

---

## 不需要新模块依赖
现有 Build.cs 已包含 Engine（DataTable, FFileHelper）、SlateCore（UMG）、GameplayTags。

## 编辑器手动操作
- 创建所有地形 DataAsset（Content/Grid/Terrain/）
- 在 BP_HexGridManager 配置 TerrainRegistry（枚举→DataAsset 映射）和 DecorationRegistry
- 创建 WBP_MapEditor UMG 蓝图并布局 UI
- 创建 IA_ToggleEditor InputAction

## 验证方式
1. 手动编写测试 CSV 文件，LoadMapFromCSV 加载非矩形地图
2. SaveMapToCSV 保存后重新加载，验证数据一致性
3. 编辑器中切换工具、点击格子修改地形、添加/移除格子
4. 保存 → 退出 → 重新加载，确认数据持久化
