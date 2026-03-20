# 城镇系统第一期：可配置建筑面板

## 设计思路

城镇节点进入后弹出 `TownWidget`，面板上显示该城镇配置的建筑图标（可点击）。建筑类型复用世界地图节点类型（商店、升华塔、传送阵等），城内外统一。每个城镇节点可独立配置拥有哪些建筑。

**初版功能：商店（占位）、升华塔（已有，直接复用）、传送阵（占位）。**

---

## Step 1：建筑类型枚举 + 城镇数据结构

### 新建文件：`Source/LFP2D/Town/LFPTownData.h`

```cpp
#pragma once
#include "CoreMinimal.h"
#include "LFPTownData.generated.h"

// 城镇建筑类型（与世界地图节点类型对齐，但专为城镇内使用）
UENUM(BlueprintType)
enum class ELFPTownBuildingType : uint8
{
    TBT_Shop            UMETA(DisplayName = "商店"),
    TBT_EvolutionTower  UMETA(DisplayName = "升华塔"),
    TBT_Teleport        UMETA(DisplayName = "传送阵"),
    TBT_QuestNPC        UMETA(DisplayName = "任务NPC"),
    TBT_SkillNode       UMETA(DisplayName = "技能节点"),
};

// 单个建筑配置
USTRUCT(BlueprintType)
struct FLFPTownBuildingEntry
{
    GENERATED_BODY()

    // 建筑类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ELFPTownBuildingType BuildingType = ELFPTownBuildingType::TBT_Shop;

    // 建筑图标（蓝图中配置）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> BuildingIcon;

    // 建筑显示名
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    // 是否可用（未解锁的建筑灰显）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnabled = true;

    // 传送阵专用：目标节点 ID 列表（分号分隔，如 "3;7;12"）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TeleportTargetNodeIDs;
};
```

**理由**：独立枚举而非复用 `ELFPWorldNodeType`，因为城镇建筑和世界节点虽然语义重合，但后续城镇可能有世界地图没有的建筑类型（如铁匠铺、酒馆），解耦更灵活。

---

## Step 2：城镇数据挂到世界地图节点

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapData.h`

`FLFPWorldNodeRow` 新增：
```cpp
// ==== 城镇节点参数 ====

// 城镇建筑配置字符串（序列化格式，如 "Shop;EvolutionTower;Teleport"）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Town")
FString TownBuildingList;
```

用分号分隔的建筑类型名字符串存入 CSV，加载时解析为 `TArray<ELFPTownBuildingType>`。这样 CSV 保持平面结构，不需要改存储架构。

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapNode.h/.cpp`

新增属性 + InitFromRowData/ExportToRowData 同步：
```cpp
// 城镇建筑列表字符串
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Town")
FString TownBuildingList;
```

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapManager.cpp`

`SaveNodesToCSV` 表头和写行追加 `TownBuildingList` 列。
同时顺手修复已知 bug：表头和写行也追加 `BaseGoldReward,BaseFoodReward`（当前漏存了）。

---

## Step 3：世界地图编辑器支持城镇建筑配置

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h/.cpp`

新增笔刷参数：
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
FString BrushTownBuildingList;

UFUNCTION(BlueprintCallable)
void SetBrushTownBuildingList(const FString& InList) { BrushTownBuildingList = InList; }
```

`ApplyParamsToSelectedNode()` 中追加：
```cpp
SelectedNode->TownBuildingList = BrushTownBuildingList;
```

### 修改：`Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h/.cpp`

新增 BindWidgetOptional：
```cpp
UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
TObjectPtr<UEditableTextBox> TownBuildingListInput;
```

绑定 OnTextCommitted → `EditorComponent->SetBrushTownBuildingList()`。
`UpdateNodeInfo()` 中，当节点类型为 Town 时显示建筑列表。

---

## Step 4：TownWidget（城镇面板 UI）

### 新建文件：`Source/LFP2D/UI/Town/LFPTownWidget.h/.cpp`

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTownWidgetClosed);

UCLASS()
class LFP2D_API ULFPTownWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初始化：传入建筑列表 + GameInstance
    UFUNCTION(BlueprintCallable)
    void Setup(const TArray<FLFPTownBuildingEntry>& Buildings, ULFPGameInstance* GI);

    // 关闭委托（仿照 UnitMergeWidget 模式）
    UPROPERTY(BlueprintAssignable)
    FOnTownWidgetClosed OnClosed;

protected:
    virtual void NativeConstruct() override;

private:
    // 点击建筑按钮
    UFUNCTION() void OnBuildingClicked();

    // 点击关闭
    UFUNCTION() void OnCloseClicked();

    // 根据建筑类型执行对应功能
    void ExecuteBuilding(ELFPTownBuildingType Type);

    // 刷新建筑图标列表
    void RefreshBuildingIcons();

protected:
    // BindWidget 控件
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TownName;

    // 建筑图标容器（动态填充建筑按钮）
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWrapBox> Box_Buildings;  // 或 UniformGridPanel

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Close;

    // 运行时数据
    UPROPERTY()
    TObjectPtr<ULFPGameInstance> CachedGameInstance;

    TArray<FLFPTownBuildingEntry> BuildingEntries;

    // 按钮 → 建筑类型映射
    UPROPERTY()
    TArray<TObjectPtr<UButton>> BuildingButtons;
    TMap<UButton*, ELFPTownBuildingType> ButtonToBuildingMap;
};
```

**核心逻辑：**
- `Setup()` 接收解析后的建筑数组，调用 `RefreshBuildingIcons()` 动态创建按钮
- `RefreshBuildingIcons()` 遍历建筑列表，为每个建筑在 `Box_Buildings` 中创建一个带图标的 Button
- `OnBuildingClicked()` 通过 `ButtonToBuildingMap` 查找类型，调用 `ExecuteBuilding()`
- `ExecuteBuilding()` 按类型分发：
  - `TBT_EvolutionTower` → 通知 PlayerController 打开升华塔（复用已有 `OpenEvolutionTower()`）
  - `TBT_Shop` → UE_LOG 占位（后续实现）
  - `TBT_Teleport` → UE_LOG 占位（后续实现）
  - 其他 → UE_LOG 占位

---

## Step 5：PlayerController 接入城镇

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h`

新增：
```cpp
// 城镇面板
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
TSubclassOf<ULFPTownWidget> TownWidgetClass;

UPROPERTY()
TObjectPtr<ULFPTownWidget> TownWidget;

// 建筑视觉注册表：类型 → 图标/显示名（蓝图配置）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town")
TMap<ELFPTownBuildingType, FLFPTownBuildingEntry> TownBuildingRegistry;

void OpenTown(ALFPWorldMapNode* TownNode);

UFUNCTION()
void OnTownWidgetClosed();

// TownWidget 请求打开子功能的委托/回调
UFUNCTION()
void OnTownBuildingRequested(ELFPTownBuildingType BuildingType);
```

### 修改：`Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`

`EnterNode()` 中：
```cpp
case ELFPWorldNodeType::WNT_Town:
    OpenTown(Node);
    break;
```

`OpenTown()` 实现：
1. 从 `Node->TownBuildingList` 解析出 `TArray<ELFPTownBuildingType>`
2. 用 `TownBuildingRegistry` 填充每个建筑的图标/显示名，组装 `TArray<FLFPTownBuildingEntry>`
3. 创建/显示 `TownWidget`，调用 `Setup()`
4. 绑定 `OnClosed` 委托

`OnTownBuildingRequested()` 实现：
- `TBT_EvolutionTower` → 隐藏 TownWidget → `OpenEvolutionTower()` → 升华塔关闭后重新显示 TownWidget
- `TBT_Shop` / `TBT_Teleport` → 占位日志

---

## Step 6：解析工具函数

### 新增到 `Source/LFP2D/Town/LFPTownData.h`（或独立 .cpp）

```cpp
// 字符串 → 建筑类型数组（"Shop;EvolutionTower;Teleport" → TArray）
static TArray<ELFPTownBuildingType> ParseTownBuildingList(const FString& InStr);

// 建筑类型数组 → 字符串
static FString SerializeTownBuildingList(const TArray<ELFPTownBuildingType>& InList);
```

---

## 文件变更汇总

| 操作 | 文件 | 说明 |
|---|---|---|
| **新建** | `Source/LFP2D/Town/LFPTownData.h` | 建筑枚举 + 数据结构 + 解析工具 |
| **新建** | `Source/LFP2D/UI/Town/LFPTownWidget.h` | 城镇面板 Widget 头文件 |
| **新建** | `Source/LFP2D/UI/Town/LFPTownWidget.cpp` | 城镇面板 Widget 实现 |
| 修改 | `Source/LFP2D/WorldMap/LFPWorldMapData.h` | FLFPWorldNodeRow +TownBuildingList |
| 修改 | `Source/LFP2D/WorldMap/LFPWorldMapNode.h/.cpp` | +TownBuildingList 属性/导入导出 |
| 修改 | `Source/LFP2D/WorldMap/LFPWorldMapManager.cpp` | CSV 存储追加列（+修复奖励漏存 bug） |
| 修改 | `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h/.cpp` | +BrushTownBuildingList |
| 修改 | `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h/.cpp` | +TownBuildingListInput |
| 修改 | `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h/.cpp` | +OpenTown/TownWidget/Registry |

---

## 蓝图/编辑器手动配置（实现后）

1. 创建 `Content/UI/Town/WBP_Town`（父类 `ULFPTownWidget`），摆放 `Text_TownName`、`Box_Buildings`、`Button_Close`
2. 在 BP_WorldMapPlayerController 上配置 `TownWidgetClass = WBP_Town`
3. 在 BP_WorldMapPlayerController 上配置 `TownBuildingRegistry`（每种建筑类型对应图标和显示名）
4. 在世界地图编辑器中为 Town 节点填写 `TownBuildingList`（如 `Shop;EvolutionTower;Teleport`）
5. 在 NodeVisualRegistry 中为 `WNT_Town` 配置视觉数据资产（如果还没有）

---

## 实施顺序

1. Step 1 → Step 2 → 编译验证数据层
2. Step 3 → 编译验证编辑器可配置
3. Step 4 → Step 5 → Step 6 → 编译验证城镇面板可弹出、升华塔可从城内打开
