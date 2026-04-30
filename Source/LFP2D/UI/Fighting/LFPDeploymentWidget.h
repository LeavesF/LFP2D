#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPDeploymentWidget.generated.h"

class ULFPUnitRegistryDataAsset;
class ULFPUnitSlotWidget;
class UWrapBox;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeploymentPartyUnitClicked, int32, PartyIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeploymentReserveUnitClicked, int32, ReserveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentConfirmed);

/**
 * 布置阶段 UI：出战/备战双行单位列表，使用 LFPUnitSlotWidget 动态填充
 */
UCLASS()
class LFP2D_API ULFPDeploymentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化 UI 数据（动态创建所有槽位 Widget）
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void Setup(const TArray<FLFPUnitEntry>& PartyUnits, const TArray<FLFPUnitEntry>& ReserveUnits, ULFPUnitRegistryDataAsset* Registry);

	// 更新单个出战槽位图标（交换/替换后调用）
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void UpdatePartySlot(int32 PartyIndex, const FLFPUnitEntry& Entry);

	// 更新单个备战槽位图标（交换/替换后调用）
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void UpdateReserveSlot(int32 ReserveIndex, const FLFPUnitEntry& Entry);

	// 高亮/取消出战单位选中
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void MarkPartyUnitSelected(int32 PartyIndex, bool bSelected);

	// 高亮/取消备战单位选中
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void MarkReserveUnitSelected(int32 ReserveIndex, bool bSelected);

	// 清除所有选中高亮
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void ClearAllSelections();

	// 委托：玩家点击了某个出战单位图标
	UPROPERTY(BlueprintAssignable)
	FOnDeploymentPartyUnitClicked OnPartyUnitClicked;

	// 委托：玩家点击了某个备战单位图标
	UPROPERTY(BlueprintAssignable)
	FOnDeploymentReserveUnitClicked OnReserveUnitClicked;

	// 委托：玩家按下确认按钮
	UPROPERTY(BlueprintAssignable)
	FOnDeploymentConfirmed OnConfirmPressed;

protected:
	virtual void NativeConstruct() override;

	// ============== BindWidget 绑定 ==============

	// 出战行容器（WrapBox，自动换行）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWrapBox> Box_Party;

	// 备战行容器（WrapBox）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWrapBox> Box_Reserve;

	// 确认按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Confirm;

	// ============== 蓝图配置 ==============

	// 单位槽位 Widget 蓝图类（必须指定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment")
	TSubclassOf<ULFPUnitSlotWidget> UnitSlotWidgetClass;

	// ============== 数据缓存 ==============
	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<FLFPUnitEntry> CachedPartyUnits;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<FLFPUnitEntry> CachedReserveUnits;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;

	// 当前选中的槽位
	int32 SelectedPartyIndex = -1;
	int32 SelectedReserveIndex = -1;

	// 动态创建的槽位 Widget
	UPROPERTY()
	TArray<TObjectPtr<ULFPUnitSlotWidget>> PartySlotWidgets;

	UPROPERTY()
	TArray<TObjectPtr<ULFPUnitSlotWidget>> ReserveSlotWidgets;

private:
	UFUNCTION() void OnConfirmClicked();

	UFUNCTION() void OnPartySlotClicked(ULFPUnitSlotWidget* ClickedSlot);
	UFUNCTION() void OnReserveSlotClicked(ULFPUnitSlotWidget* ClickedSlot);

	// 创建一行槽位（bIsParty: true=出战行, false=备战行）
	void CreateSlots(const TArray<FLFPUnitEntry>& Units, UWrapBox* Container, TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets, bool bIsParty);

	// 清除容器内所有子控件
	void ClearContainer(UWrapBox* Container, TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets);
};
