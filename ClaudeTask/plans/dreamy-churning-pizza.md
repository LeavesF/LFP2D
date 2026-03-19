# 世界地图添加升华塔节点

## Context

玩家需要在世界地图上有一个专门的节点类型"升华塔"用于单位升阶。到达该节点后打开 WBP_UnitMerge 面板。

## 改动

### 1. `Source/LFP2D/WorldMap/LFPWorldMapData.h` — 新增枚举值

`ELFPWorldNodeType` 添加：
```cpp
WNT_EvolutionTower UMETA(DisplayName = "升华塔")
```

### 2. `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h` — 新增成员

```cpp
// 升阶面板蓝图类
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
TSubclassOf<ULFPUnitMergeWidget> UnitMergeWidgetClass;

UPROPERTY()
TObjectPtr<ULFPUnitMergeWidget> UnitMergeWidget;

// 打开升阶面板
void OpenEvolutionTower();

// 升阶面板关闭回调
UFUNCTION()
void OnUnitMergeWidgetClosed();
```

### 3. `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp` — 实现

`EnterNode()` switch 中添加：
```cpp
case ELFPWorldNodeType::WNT_EvolutionTower:
    OpenEvolutionTower();
    break;
```

`OpenEvolutionTower()` 实现：创建 UnitMergeWidget，调用 `Setup(GI, GI->UnitRegistry)`，绑定 `OnClosed` 委托。

### 4. 蓝图配置（手动）

- 在 NodeVisualRegistry 中为 `WNT_EvolutionTower` 配置视觉数据资产
- 在世界地图 CSV 中添加升华塔节点

## 验证

编译通过后，在世界地图编辑器中放置一个 WNT_EvolutionTower 节点，玩家移动到该节点后应自动打开升阶面板。
