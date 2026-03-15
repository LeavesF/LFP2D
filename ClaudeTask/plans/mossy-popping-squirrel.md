# 六角格地形分层 Alpha 过渡系统设计方案

## 背景

当前 HexTile 每个格子使用独立的 PaperSprite 渲染地形，相同地形的相邻格子看起来是重复贴图，不同地形相邻时边缘生硬。目标是实现：
1. **同种地形连续感**：使用 World-Space UV，同类地形格子共享一张大纹理的不同部分
2. **异种地形自然过渡**：高优先级地形沿边缘"渗透"到低优先级地形上，产生柔和的 Alpha 渐变

## 整体架构

```
┌─────────────────────────────────────────────────┐
│                ALFPHexTile                       │
│                                                  │
│  SpriteComponent (Z=0)                          │
│    └─ M_TerrainBase 材质 (World-Space UV)        │
│                                                  │
│  TransitionComponents[0~5] (Z=0.15)  ← 新增     │
│    └─ M_TerrainTransition 材质 (Alpha 遮罩)      │
│                                                  │
│  DecorationSpriteComponent (Z=0.5)              │
│  OverlaySpriteComponent (Z=0.3)                 │
│  EdgeSpriteComponents[0~5] (Z=1.0)             │
└─────────────────────────────────────────────────┘
```

过渡逻辑：
```
格子 A（草地，优先级 1）  ←→  格子 B（石地，优先级 3）

格子 A 的东方向 TransitionComponent：显示，采样石地纹理，遮罩朝东
格子 B 的西方向 TransitionComponent：隐藏（草地优先级更低，不渗透）
```

## 需要修改的文件

### 1. `Source/LFP2D/HexGrid/LFPTerrainDataAsset.h` — 数据扩展

新增两个属性：

```cpp
// 渲染优先级（数值越大越优先，高优先级地形覆盖低优先级边缘）
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
int32 RenderPriority = 0;

// 地形 Tileable 纹理（用于 World-Space UV 连续渲染）
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
TObjectPtr<UTexture2D> TerrainTexture = nullptr;
```

建议优先级配置：草地=0, 沙地=1, 泥地=2, 石地=3, 雪地=4, 水域=5, 熔岩=6, 魔法=7, 桥=3

### 2. `Source/LFP2D/HexGrid/LFPHexTile.h/.cpp` — 过渡组件

**新增成员：**
```cpp
// ============== 地形过渡系统 ==============

// 6 个过渡精灵组件（每个方向一个）
UPROPERTY(VisibleAnywhere, Category = "Components")
TArray<TObjectPtr<UPaperSpriteComponent>> TransitionComponents;

// 基础地形动态材质实例
UPROPERTY()
TObjectPtr<UMaterialInstanceDynamic> BaseMID;

// 6 个过渡层动态材质实例
UPROPERTY()
TArray<TObjectPtr<UMaterialInstanceDynamic>> TransitionMIDs;
```

**新增方法：**
```cpp
// 初始化过渡组件（由 GridManager 在生成格子后调用）
void InitializeTransitionComponents(UPaperSprite* HexSprite,
    UMaterialInterface* BaseTerrainMat, UMaterialInterface* TransitionMat,
    float TextureScale);

// 更新基础地形材质（设置纹理参数）
void UpdateBaseMaterial(UTexture2D* TerrainTexture, float TextureScale);

// 更新指定方向的过渡（DirIndex 对应 HexDirections）
void ShowTransition(int32 DirIndex, UTexture2D* NeighborTexture, float TextureScale);
void HideTransition(int32 DirIndex);
void ClearAllTransitions();
```

**构造函数中创建 6 个 TransitionComponents：**
- Z = 0.15（在 base Z=0 和 overlay Z=0.3 之间）
- 初始隐藏
- 使用与 SpriteComponent 相同的六角精灵

**InitializeTransitionComponents 逻辑：**
1. 为 SpriteComponent 创建 BaseMID（从 BaseTerrainMat），设置 TerrainTexture 参数
2. 为每个 TransitionComponent 创建 TransitionMID（从 TransitionMat），设置 MaskAngle 参数
3. MaskAngle 映射：DirIdx 0→0°, 1→60°, 2→120°, 3→180°, 4→240°, 5→300°

**SetTerrainData 修改：**
- 在现有逻辑后，如果有 BaseMID，更新 TerrainTexture 参数

### 3. `Source/LFP2D/HexGrid/LFPHexGridManager.h/.cpp` — 过渡编排

**新增属性：**
```cpp
// ============== 地形过渡系统 ==============

// 基础地形材质（World-Space UV）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
TObjectPtr<UMaterialInterface> TerrainBaseMaterial;

// 过渡层材质（Alpha 遮罩混合）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
TObjectPtr<UMaterialInterface> TerrainTransitionMaterial;

// 过渡层精灵（六角形，覆盖整个格子区域）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
TObjectPtr<UPaperSprite> TransitionHexSprite;

// 纹理缩放（控制 World-Space UV 的纹理密度）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition", meta = (ClampMin = "0.1"))
float TerrainTextureScale = 200.0f;

// 是否启用过渡效果
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
bool bEnableTerrainTransition = true;
```

**新增方法：**
```cpp
// 更新单个格子的所有过渡
void UpdateTileTransitions(ALFPHexTile* Tile);

// 更新所有格子的过渡
void UpdateAllTransitions();

// 更新指定坐标及其邻居的过渡（用于编辑器）
void UpdateTransitionsAround(int32 Q, int32 R);
```

**SpawnTileAt 修改：**
- 在 `InitializeEdgeComponents()` 之后调用 `InitializeTransitionComponents()`
- 不在此处触发过渡更新（等整个地图加载完再批量更新）

**GenerateGrid / LoadMapFromDataTable 修改：**
- 在所有格子生成完成后，调用 `UpdateAllTransitions()`

**UpdateTileTransitions 核心逻辑：**
```
void UpdateTileTransitions(ALFPHexTile* Tile):
    如果 Tile 没有 TerrainData 或不启用过渡 → ClearAllTransitions() 返回

    先更新基础材质纹理参数

    For DirIdx = 0 to 5:
        获取方向 DirIdx 的邻居 Neighbor
        如果 Neighbor 不存在或 Neighbor 地形类型相同:
            Tile->HideTransition(DirIdx)
        否则如果 Neighbor 的 RenderPriority > Tile 的 RenderPriority:
            Tile->ShowTransition(DirIdx, Neighbor 的 TerrainTexture, TextureScale)
        否则:
            Tile->HideTransition(DirIdx)  // 低优先级不渗透
```

**UpdateTransitionsAround 逻辑：**
```
更新 (Q,R) 处的格子
遍历 6 个方向，更新每个邻居
（因为修改一个格子的地形会影响它自己和所有邻居的过渡）
```

### 4. `Source/LFP2D/HexGrid/LFPMapEditorComponent.cpp` — 编辑器集成

在 `ApplyToolToTile` 的 `MET_Terrain` 分支末尾添加：
```cpp
GM->UpdateTransitionsAround(Tile->GetCoordinates().Q, Tile->GetCoordinates().R);
```

在 `ApplyToolToCoord` 的 `MET_AddTile` 分支末尾添加：
```cpp
GM->UpdateTransitionsAround(Q, R);
```

在 `ApplyToolToTile` 的 `MET_RemoveTile` 分支中，在 RemoveTile 之前保存坐标，之后更新邻居过渡。

### 5. 材质资产（在编辑器中创建）

#### M_TerrainBase（不透明材质）
```
节点连接：
[AbsoluteWorldPosition] → [ComponentMask R,G] → [Divide by TextureScale] → [TextureSample UV]
                                                                              ↓
TextureSample → BaseColor 输出

参数：
- TerrainTexture (Texture2D): 地形 Tileable 纹理
- TextureScale (Scalar, 默认 200): 纹理缩放
```

#### M_TerrainTransition（半透明材质）
```
混合模式: Translucent

节点连接：
--- 地形纹理采样（同 M_TerrainBase）---
[AbsoluteWorldPosition] → [ComponentMask R,G] → [Divide by TextureScale] → [TextureSample UV]
                                                                              ↓
TextureSample → BaseColor 输出

--- 过渡遮罩 ---
[TextureCoordinate] → [Subtract (0.5, 0.5)] → [CustomRotator by MaskAngle] → [Add (0.5, 0.5)]
                                                                                 ↓
                                                                    [TransitionMask 纹理采样]
                                                                                 ↓
                                                                         Opacity 输出

参数：
- NeighborTexture (Texture2D): 邻居地形纹理
- TextureScale (Scalar): 与基础层一致
- MaskAngle (Scalar): 遮罩旋转角度 (0°/60°/120°/180°/240°/300°)
```

### 6. 美术资产

#### T_TransitionMask（过渡遮罩纹理）
- 尺寸：256×256 或 512×512
- 灰度图
- 右侧 ~40% 区域：从白色(1.0)渐变到黑色(0.0)
- 左侧 ~60% 区域：全黑(0.0)
- 整体形状：可以是矩形（六角精灵的多边形裁剪会处理边界）
- 需要是六角形区域内的扇形渐变效果更佳

#### 每种地形的 Tileable 纹理
- 草地、沙地、泥地、石地、雪地等各一张
- 必须是四方连续（Seamless Tileable）纹理
- 建议尺寸：512×512 或 1024×1024

## 性能分析

- 每个格子新增 6 个 TransitionComponents（PaperSpriteComponent）
- 大多数组件隐藏（Hidden），不产生绘制开销
- 典型地图 300 格子，假设 30% 是地形边界 ≈ 90 格子 × 平均 2 个过渡 = ~180 个可见过渡精灵
- Paper2D 2D 场景下完全可以承受
- `bEnableTerrainTransition` 开关可随时关闭整个系统

## 向后兼容

- 如果 TerrainData 没有设置 TerrainTexture，系统自动降级为原有的 DefaultSprite 渲染
- 如果 TerrainBaseMaterial / TerrainTransitionMaterial 未配置，跳过过渡初始化
- 现有的高亮系统（Overlay、Edge）完全不受影响（Z 层级分离）

## 验证方法

1. 构建项目，确认编译通过
2. 在编辑器中创建 M_TerrainBase 和 M_TerrainTransition 材质
3. 制作或导入 T_TransitionMask 遮罩纹理
4. 为 DA_Terrain_Grass / DA_Terrain_Sand 等数据资产配置 RenderPriority 和 TerrainTexture
5. 在 BP_HexGridManager 蓝图中配置材质引用和 TransitionHexSprite
6. 运行地图编辑器，绘制相邻的不同地形，观察：
   - 同种地形是否连续无缝
   - 不同地形边缘是否有自然的 Alpha 渐变过渡
   - 切换地形时过渡是否实时更新
