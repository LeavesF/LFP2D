#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPDeploymentWidget.generated.h"

class ULFPUnitRegistryDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeploymentUnitSelected, int32, PartyIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentConfirmed);

/**
 * 布置阶段 UI：显示队伍单位列表，玩家点击选中后放置到出生点
 * 具体布局由蓝图子类实现
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

	// 确认按钮点击（蓝图中绑定按钮调用此方法）
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void OnConfirmButtonClicked();

	// 单位图标点击（蓝图中绑定按钮调用此方法）
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void OnUnitIconClicked(int32 PartyIndex);

protected:
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

	// 蓝图实现：刷新 UI 显示
	UFUNCTION(BlueprintImplementableEvent, Category = "Deployment")
	void OnSetupComplete();

	// 蓝图实现：单位放置状态变化时刷新
	UFUNCTION(BlueprintImplementableEvent, Category = "Deployment")
	void OnPlacedStateChanged(int32 PartyIndex, bool bPlaced);

	// 蓝图实现：确认按钮状态变化
	UFUNCTION(BlueprintImplementableEvent, Category = "Deployment")
	void OnConfirmEnabledChanged(bool bEnabled);
};
