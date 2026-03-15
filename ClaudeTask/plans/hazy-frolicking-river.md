# 六边形范围高亮系统重构：边缘描边 + 半透明填充

## 背景

当前移动/攻击/技能范围的显示方式是**直接替换整个格子的 Sprite**（DefaultSprite ↔ MovementRangeSprite 等），这导致地形纹理完全丢失、每种地形需要多套高亮贴图、无法动态调色。

新方案改为：在每个 HexTile 上叠加**6 个边线 Sprite（描边）** + **1 个半透明覆盖 Sprite（填充）**，地形 Sprite 永远不变。

## 涉及文件

| 文件 | 变更类型 |
|------|----------|
| `Source/LFP2D/HexGrid/LFPHexTile.h` | 新增组件和方法，删除旧高亮属性/方法 |
| `Source/LFP2D/HexGrid/LFPHexTile.cpp` | 实现新高亮系统，删除旧实现 |
| `Source/LFP2D/HexGrid/LFPHexGridManager.h` | 新增高亮 API 和精灵资产引用 |
| `Source/LFP2D/HexGrid/LFPHexGridManager.cpp` | 实现边界检测算法和新高亮方法 |
| `Source/LFP2D/Player/LFPTacticsPlayerController.cpp` | 迁移所有调用点到新 API |

## 实现步骤

### 步骤 1：修改 LFPHexTile — 新增组件和方法

**LFPHexTile.h：**
- 新增 `TArray<TObjectPtr<UPaperSpriteComponent>> EdgeSpriteComponents`（6个边线精灵）
- 新增 `TObjectPtr<UPaperSpriteComponent> OverlaySpriteComponent`（半透明填充层）
- 新增方法：
  - `InitializeEdgeComponents(UPaperSprite* EdgeSprite, UPaperSprite* OverlaySprite, float HexSize, float VerticalScale)` — GridManager 创建格子后调用
  - `ShowEdge(int32 DirIndex, FLinearColor Color)` — 显示某条边
  - `HideEdge(int32 DirIndex)` — 隐藏某条边
  - `ShowRangeOverlay(EUnitRange RangeType)` — 显示填充覆盖层
  - `ShowPathOverlay(bool bActive)` — 路径高亮
  - `ClearAllHighlights()` — 清除所有高亮
- 删除旧属性：`PathSprite`, `MovementRangeSprite`, `AttackRangeSprite`, `SkillEffectRangeSprite`
- 删除旧方法：`SetRangeSprite()`, `SetMovementHighlight()`, `SetAttackHighlight()`, `SetPathHighlight()`
- 保留 `DefaultSprite`（仍由 SetTerrainData 设置地形图，但不再用于高亮还原）

**LFPHexTile.cpp：**
- 构造函数中创建 OverlaySpriteComponent（Z=0.3f，默认隐藏）和 6 个 EdgeSpriteComponents（默认隐藏）
- `InitializeEdgeComponents()` 计算每条边的位置和旋转：
  - 平顶六边形顶点角度 = 60° × i + 30°（与 DrawDebugHexagon 一致）
  - 顶点 i: (HexSize × cos(angle), HexSize × VerticalScale × sin(angle))
  - 边 i 中点 = (顶点i + 顶点(i+1)%6) / 2，Z = 1.0f
  - 边方向角度 = atan2(dy, dx)
  - Paper2D 旋转：Roll = EdgeAngle（XY 平面内旋转）
- 颜色映射：
  - UR_Move: 边线蓝色(0.2,0.5,1.0,0.8)，填充(0.2,0.5,1.0,0.25)
  - UR_Attack: 边线红色(1.0,0.2,0.2,0.8)，填充(1.0,0.2,0.2,0.25)
  - UR_SkillEffect: 边线绿色(0.2,1.0,0.4,0.8)，填充(0.2,1.0,0.4,0.25)
  - 路径: 填充亮黄(1.0,0.9,0.3,0.35)
- 删除 `SetRangeSprite`, `SetMovementHighlight`, `SetAttackHighlight`, `SetPathHighlight` 实现
- `SetTerrainData` 不再引用 `DefaultSprite` 做高亮还原，只做一次性地形设置

### 步骤 2：修改 LFPHexGridManager — 新增高亮 API

**LFPHexGridManager.h：**
- 新增属性：
  - `UPaperSprite* EdgeHighlightSprite` — 边线精灵（通用，蓝图中配置）
  - `UPaperSprite* RangeOverlaySprite` — 六边形覆盖层精灵（蓝图中配置）
- 新增方法：
  - `ShowRangeHighlight(const TArray<ALFPHexTile*>& RangeTiles, EUnitRange RangeType)` — 显示范围（带边界检测）
  - `ShowRangeHighlightByCoords(const TArray<FLFPHexCoordinates>& Coords, EUnitRange RangeType)` — 坐标版
  - `ShowPathHighlight(const TArray<ALFPHexTile*>& PathTiles)` — 路径高亮
  - `ClearAllHighlights()` — 清除所有
  - `ClearPathHighlight()` — 仅清除路径高亮
- 新增缓存：`TArray<ALFPHexTile*> CurrentHighlightedTiles`, `CurrentPathTiles`
- 删除旧方法：`UpdateGridSpriteWithTiles`, `UpdateGridSpriteWithCoords`, `ResetGridSprite`

**LFPHexGridManager.cpp：**
- `SpawnTileAt()` 末尾调用 `NewTile->InitializeEdgeComponents(...)`
- `ShowRangeHighlight()` 核心逻辑：
  1. `ClearAllHighlights()`
  2. 建立范围 TSet<FIntPoint> 用于 O(1) 查找
  3. 对每个范围格子：显示覆盖层 + 检查 6 个邻居，不在范围内的方向显示边线
  4. 缓存到 CurrentHighlightedTiles
- `ShowPathHighlight()`: 对路径格子设置路径覆盖层颜色
- `ClearAllHighlights()`: 仅清除已缓存的格子（不遍历全图），效率更高
- `ClearPathHighlight()`: 仅清路径格子的路径覆盖层，恢复为范围覆盖层
- 辅助方法 `GetEdgeColorForRange(EUnitRange)` / `GetOverlayColorForRange(EUnitRange)` 返回颜色

### 步骤 3：更新 PlayerController 调用点

**LFPTacticsPlayerController.cpp** 中需要迁移的调用点（共约 8 处）：

1. **ShowUnitRange()** (L629): `GridManager->ResetGridSprite()` → `GridManager->ClearAllHighlights()`
2. **ShowUnitRange()** (L649): `GridManager->UpdateGridSpriteWithTiles(...)` → `GridManager->ShowRangeHighlight(MovementRangeTiles, UR_Move)`
3. **ShowUnitRange()** (L659): `Tile->SetRangeSprite(UR_Attack)` 循环 → `GridManager->ShowRangeHighlight(CacheRangeTiles, UR_Attack)`
4. **ShowPathToSelectedTile()** (L682): `Tile->SetPathHighlight(true)` 循环 → `GridManager->ShowPathHighlight(CurrentPath)`
5. **HidePathToDefault()** (L691): `Tile->SetMovementHighlight(false)` → `GridManager->ClearPathHighlight()` + `GridManager->ClearAllHighlights()`
6. **HidePathToRange()** (L700): `Tile->SetMovementHighlight(true)` → `GridManager->ClearPathHighlight()`
7. **HandleSkillTargetSelecting()** (L892): `GridManager->UpdateGridSpriteWithCoords(...)` → `GridManager->ShowRangeHighlightByCoords(TargetTilesCoord, UR_SkillEffect)`
8. **ShowEnemyPlanPreview()** (L939): `Tile->SetRangeSprite(UR_SkillEffect)` → `GridManager->ShowRangeHighlight(PreviewEffectTiles, UR_SkillEffect)`
9. **HideEnemyPlanPreview()** (L951): `Tile->SetRangeSprite(UR_Default)` → `GridManager->ClearAllHighlights()`
10. **OnDeploymentPhaseStarted()** (L1038): `Tile->SetMovementHighlight(true)` → `GridManager->ShowRangeHighlight(PlayerSpawnTiles, UR_Move)`
11. **ConfirmDeployment()** (L1201): `Tile->SetMovementHighlight(false)` → `GridManager->ClearAllHighlights()`

### 步骤 4：美术资产（手动操作）

实现完成后需要在编辑器中：
1. 创建 **边线贴图**（白色细矩形，如 100×4px）→ 导入为 PaperSprite `S_HexEdge`
2. 创建 **六边形覆盖层贴图**（白色六边形，匹配格子形状）→ 导入为 PaperSprite `S_HexOverlay`
3. 在 GridManager 蓝图上配置 `EdgeHighlightSprite` 和 `RangeOverlaySprite`

## Z 层级（从下到上）

| 层 | Z 偏移 | 内容 |
|----|--------|------|
| 地形 | 0 | SpriteComponent（永不改变） |
| 覆盖层 | 0.3f | OverlaySpriteComponent（半透明填充） |
| 装饰 | 0.5f | DecorationSpriteComponent（已有） |
| 边线 | 1.0f | EdgeSpriteComponents × 6 |
| 单位 | ~50f | Unit Actor |

## 验证方式

1. 编译通过
2. 在编辑器中配置边线和覆盖层精灵到 GridManager
3. 运行游戏，选中单位 → 应看到蓝色半透明填充 + 边界蓝色描边
4. 点击范围内格子 → 路径以亮色覆盖层显示
5. 切换到攻击模式 → 红色描边 + 红色填充
6. 技能释放 → 绿色描边 + 绿色填充
7. 布置阶段 → 出生点有蓝色高亮
