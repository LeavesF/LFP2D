#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPUnitMergeWidget.generated.h"

class ULFPUnitRegistryDataAsset;
class UButton;
class UImage;
class UTextBlock;
class UHorizontalBox;

// 合成框槽位信息
USTRUCT(BlueprintType)
struct FLFPMergeSlotInfo
{
	GENERATED_BODY()

	// 来源是否为出战队伍（false=备战营）
	UPROPERTY(BlueprintReadOnly)
	bool bIsParty = true;

	// 在来源数组中的索引
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;

	// 单位数据
	UPROPERTY(BlueprintReadOnly)
	FLFPUnitEntry Unit;

	// 是否为空
	UPROPERTY(BlueprintReadOnly)
	bool bIsEmpty = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMergeWidgetClosed);

/**
 * 单位升阶 UI Widget
 * 三框布局：上方预览框 + 下方两个合成框 + 底部队伍/备战营 icon 栏
 * 点击底部 icon 放入合成框，两框匹配时显示预览，确认后合并
 */
UCLASS()
class LFP2D_API ULFPUnitMergeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化（传入 GameInstance 和注册表）
	UFUNCTION(BlueprintCallable, Category = "Unit Merge")
	void Setup(ULFPGameInstance* GI, ULFPUnitRegistryDataAsset* Registry);

	// 关闭委托
	UPROPERTY(BlueprintAssignable)
	FOnMergeWidgetClosed OnClosed;

protected:
	virtual void NativeOnInitialized() override;

private:
	// 刷新底部单位 icon 栏
	void RefreshUnitIcons();

	// 将单位放入第一个空的合成框
	void PlaceUnitInSlot(bool bIsParty, int32 Index);

	// 底部 icon 按钮点击回调（通过映射表查找对应单位）
	UFUNCTION() void OnUnitIconClicked();

	// 点击合成框 → 清空该框
	UFUNCTION() void OnSlotAClicked();
	UFUNCTION() void OnSlotBClicked();

	// 清空两个合成框
	void ClearSlots();

	// 检查匹配并更新预览
	void UpdatePreview();

	// 确认合并
	UFUNCTION() void OnMergeClicked();

	// 分支选择按钮点击
	UFUNCTION() void OnEvolutionChoiceClicked();

	// 清空按钮
	UFUNCTION() void OnClearClicked();

	// 关闭
	UFUNCTION() void OnCloseClicked();

	// 更新合成框的图标显示
	void UpdateSlotVisual(const FLFPMergeSlotInfo& InSlot, UImage* SlotImage);

	// 生成星级文本（如 "★★☆"）
	FString MakeTierStars(int32 Tier) const;

protected:
	// ==== BindWidget 控件 ====

	// 进化目标展示容器（单目标/多分支统一用此容器）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Box_EvolutionChoices;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PreviewName;

	// 合成框（可点击清空）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SlotA;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SlotA;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SlotB;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SlotB;

	// 操作按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Merge;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Clear;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	// 底部单位 icon 容器
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Box_PartyUnits;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Box_ReserveUnits;

	// 标题
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	// ==== 运行时数据 ====

	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGameInstance;

	UPROPERTY()
	TObjectPtr<ULFPUnitRegistryDataAsset> CachedRegistry;

	// 合成框状态
	FLFPMergeSlotInfo SlotA;
	FLFPMergeSlotInfo SlotB;

	// 当前选中的进化目标（分支选择时使用）
	FName SelectedEvolutionTarget;

	// 动态创建的底部 icon 按钮缓存（防止 GC）
	UPROPERTY()
	TArray<TObjectPtr<UButton>> UnitIconButtons;

	// 分支选择按钮缓存
	UPROPERTY()
	TArray<TObjectPtr<UButton>> EvolutionChoiceButtons;

	// 按钮 → 单位来源映射（bIsParty, SlotIndex）
	struct FButtonUnitMapping
	{
		bool bIsParty;
		int32 SlotIndex;
	};
	TMap<UButton*, FButtonUnitMapping> ButtonToUnitMap;

	// 分支选择按钮 → 目标 TypeID 映射
	TMap<UButton*, FName> EvolutionButtonToTargetMap;
};
