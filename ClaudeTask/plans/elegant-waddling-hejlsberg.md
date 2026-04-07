# WorldMap HUD 实现计划

## Context

世界地图目前缺少一个常驻 HUD 显示玩家资源状态。需要新增一个 HUD Widget 显示：金币、食物、回合天数（当前/阈值）、已拥有遗物图标列表。HUD 默认显示，支持按键切换可见性。布局由 UMG 蓝图自定义（代码只定义 BindWidgetOptional 控件）。

## 数据来源

| 数据 | 来源 | 获取方式 |
|------|------|---------|
| 金币 | `ULFPGameInstance::Gold` | `GI->GetGold()` |
| 食物 | `ULFPGameInstance::Food` | `GI->GetFood()` |
| 回合 | `ULFPWorldMapPlayerState::CurrentTurn/TurnPressureThreshold` | `Manager->GetPlayerState()` |
| 遗物 | `ULFPGameInstance::OwnedRelicIDs` + `RelicDataAsset` | `GI->GetOwnedRelicIDsArray()` + `GI->FindRelicDefinition()` |

## 修改/新增文件

### 1. GameInstance — 新增资源变动委托

**文件**: `Source/LFP2D/Core/LFPGameInstance.h/.cpp`

新增两个委托：
- `FOnResourceChanged(int32 NewGold, int32 NewFood)` — 在 `AddGold()`, `SpendGold()`, `AddFood()` 中广播
- `FOnOwnedRelicsChanged()` — 在 `TryAddOwnedRelic()` 成功时广播

> 注意：`TryPurchaseRelic()` 内部调用 `SpendGold()` + `TryAddOwnedRelic()`，各自会广播，无需重复。

### 2. 新建 HUD Widget

**新文件**: `Source/LFP2D/UI/WorldMap/LFPWorldMapHUDWidget.h/.cpp`

```
UCLASS()
class ULFPWorldMapHUDWidget : public UUserWidget
{
    // 初始化
    void Setup(ULFPGameInstance* GI, ULFPWorldMapPlayerState* PS);
    
    // BindWidgetOptional（布局完全由蓝图决定）
    UTextBlock* Text_Gold;
    UTextBlock* Text_Food;
    UTextBlock* Text_Turn;
    UWrapBox* Box_RelicList;  // 遗物图标容器
    
    // 刷新方法
    void RefreshAll();
    void RefreshResources();
    void RefreshTurnInfo();
    void RefreshRelicList();
    
    // 委托回调（UFUNCTION）
    void OnResourceChanged(int32 NewGold, int32 NewFood);
    void OnOwnedRelicsChanged();
    void OnTurnChanged(int32 CurrentTurn, int32 TurnBudget);
    
    // NativeDestruct 中解绑委托（GI 跨关卡持久，必须解绑）
};
```

遗物图标：`RefreshRelicList()` 中 `Box_RelicList->ClearChildren()`，遍历 `OwnedRelicIDs`，为每个遗物用 `FLFPRelicDefinition::Icon` 创建 `UImage`，通过 `SetToolTipText` 显示名称和描述。

### 3. 修改 WorldMapPlayerController

**文件**: `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h/.cpp`

新增：
- `TSubclassOf<ULFPWorldMapHUDWidget> WorldMapHUDWidgetClass`（EditAnywhere）
- `TObjectPtr<ULFPWorldMapHUDWidget> WorldMapHUDWidget`
- `UInputAction* ToggleHUDAction`（EditAnywhere）
- `void OnToggleHUDAction(const FInputActionValue& Value)`

BeginPlay 中：
- 创建 HUD Widget → `AddToViewport(0)`（低 Z-order，弹窗在上层）
- 获取 GI 和 PlayerState → `Setup(GI, PS)`

> **时序注意**：`Manager->GetPlayerState()` 可能在 BeginPlay 时尚未就绪（GameMode::StartPlay 中初始化），需要延迟到首次 Tick 或用 `TryInitializeHUD()` 模式。

SetupInputComponent 中：
- 绑定 `ToggleHUDAction` → 切换 `Visible/Collapsed`

### 4. 输入映射（蓝图侧）

需要在 UE 编辑器中：
1. 创建 `IA_ToggleHUD` InputAction 资产
2. 在 `IMC_WorldMap` 中绑定到按键（如 Tab）
3. 在 `BP_WorldMapPC` 中配置 `ToggleHUDAction` 指向该 InputAction

## 边界情况

- **关卡切换后重入世界地图**：GI 持久但 PS 重建。HUD Widget 随关卡销毁，`NativeDestruct()` 中必须解绑 GI 委托，否则 GI 保留野指针回调。
- **战斗返回后状态恢复**：GameMode 先调用 `GI->AddGold/AddFood` 再创建 HUD，所以 `Setup()` 中必须先调用 `RefreshAll()` 拉取最新值。
- **弹窗共存**：HUD 使用低 Z-order（0），商店等弹窗默认更高，不冲突。
- **遗物图标溢出**：使用 `UWrapBox` 自动换行。

## 实现步骤

1. GameInstance 添加委托声明 + 广播逻辑
2. 创建 `LFPWorldMapHUDWidget.h/.cpp`（Setup + Refresh + 委托回调 + NativeDestruct 解绑）
3. 修改 `LFPWorldMapPlayerController`（HUD 属性 + BeginPlay 创建 + ToggleHUD 输入）
4. 编译验证

## 验证

1. `Build.bat` 编译通过
2. 编辑器创建 `WBP_WorldMapHUD` 蓝图，添加命名控件
3. `BP_WorldMapPC` 配置 HUD Widget Class 和 Toggle 输入
4. 运行世界地图：确认 HUD 初始显示、Toggle 切换、移动后回合更新、商店购买后金币/遗物更新
