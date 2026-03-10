#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPDeploymentWidget.generated.h"

class ULFPUnitRegistryDataAsset;
class UButton;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeploymentUnitSelected, int32, PartyIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentConfirmed);

/**
 * 布置阶段 UI：显示队伍单位列表，玩家点击选中后放置到出生点
 */
UCLASS()
class LFP2D_API ULFPDeploymentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化 UI 数据
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void Setup(const TArray<FLFPUnitEntry>& PartyUnits, ULFPUnitRegistryDataAsset* Registry);

	// 标记某单位已放置/未放置
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void MarkUnitPlaced(int32 PartyIndex, bool bPlaced);

	// 设置确认按钮是否可用
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void SetConfirmEnabled(bool bEnabled);

	// 标记某单位正在被选中拖拽
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void MarkUnitSelecting(int32 PartyIndex);

	// 委托：玩家点击了某个单位图标
	UPROPERTY(BlueprintAssignable)
	FOnDeploymentUnitSelected OnUnitSelected;

	// 委托：玩家按下确认按钮
	UPROPERTY(BlueprintAssignable)
	FOnDeploymentConfirmed OnConfirmPressed;

protected:
	virtual void NativeConstruct() override;

	// ============== BindWidget 绑定 ==============

	// 3 个单位按钮
	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UButton> Button_Unit0;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UButton> Button_Unit1;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UButton> Button_Unit2;

	// 3 个单位图标
	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UImage> Image_Unit0;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UImage> Image_Unit1;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UImage> Image_Unit2;

	// 确认按钮
	UPROPERTY(BlueprintReadOnly, Category = "Deployment", meta = (BindWidget))
	TObjectPtr<UButton> Button_Confirm;

	// ============== 数据 ==============

	// 队伍数据缓存
	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<FLFPUnitEntry> CachedPartyUnits;

	// 注册表引用
	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;

	// 各单位是否已放置
	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<bool> PlacedStates;

	// 确认按钮是否可用
	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	bool bConfirmEnabled = false;

private:
	// 按钮点击回调
	UFUNCTION() void OnUnit0Clicked();
	UFUNCTION() void OnUnit1Clicked();
	UFUNCTION() void OnUnit2Clicked();
	UFUNCTION() void OnConfirmClicked();

	// 获取按钮/图标数组访问
	UButton* GetUnitButton(int32 Index) const;
	UImage* GetUnitImage(int32 Index) const;

	// 更新单位槽位的视觉状态
	void UpdateSlotVisual(int32 Index);
};
