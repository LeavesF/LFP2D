#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPUnitReplacementWidget.generated.h"

class ULFPUnitRegistryDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReplacementCompleteSignature);

/**
 * 单位替换 UI：当队伍和备战营都满时，展示新单位和当前阵容供玩家选择替换或放弃
 * 具体布局由蓝图子类实现
 */
UCLASS()
class LFP2D_API ULFPUnitReplacementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化 UI 数据
	UFUNCTION(BlueprintCallable, Category = "Replacement UI")
	void Setup(
		const FLFPUnitEntry& NewUnit,
		const TArray<FLFPUnitEntry>& CurrentParty,
		const TArray<FLFPUnitEntry>& CurrentReserve,
		ULFPUnitRegistryDataAsset* Registry
	);

	// 替换队伍中指定槽位的单位
	UFUNCTION(BlueprintCallable, Category = "Replacement UI")
	void ReplacePartySlot(int32 SlotIndex);

	// 替换备战营中指定槽位的单位
	UFUNCTION(BlueprintCallable, Category = "Replacement UI")
	void ReplaceReserveSlot(int32 SlotIndex);

	// 放弃新单位
	UFUNCTION(BlueprintCallable, Category = "Replacement UI")
	void DiscardNewUnit();

	// 完成委托（替换或放弃后广播）
	UPROPERTY(BlueprintAssignable)
	FOnReplacementCompleteSignature OnReplacementComplete;

protected:
	// 待加入的新单位
	UPROPERTY(BlueprintReadOnly, Category = "Replacement UI")
	FLFPUnitEntry PendingUnit;

	// 注册表引用（用于获取图标、名称）
	UPROPERTY(BlueprintReadOnly, Category = "Replacement UI")
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;
};
