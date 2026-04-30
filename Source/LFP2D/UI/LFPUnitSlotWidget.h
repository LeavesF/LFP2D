#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPUnitSlotWidget.generated.h"

class ULFPUnitRegistryDataAsset;
class UButton;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSlotClicked, ULFPUnitSlotWidget*, ClickedSlot);

/**
 * 可复用的单位图标槽位：Button + Image 组合
 * 用于部署界面、编队界面、合成界面等所有需要展示单位图标的列表
 */
UCLASS()
class LFP2D_API ULFPUnitSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 设置槽位显示的单位数据
	void Setup(const FLFPUnitEntry& Entry, ULFPUnitRegistryDataAsset* Registry);

	// 清空槽位（隐藏按钮）
	void Clear();

	// 设置选中/高亮状态
	void SetSelected(bool bSelected);

	// 是否为占用状态
	bool IsOccupied() const { return CachedEntry.IsValid(); }

	// 获取缓存的单位数据
	const FLFPUnitEntry& GetUnitEntry() const { return CachedEntry; }

	// 点击事件（广播自身指针）
	UPROPERTY(BlueprintAssignable)
	FOnUnitSlotClicked OnClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Unit;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_Unit;

private:
	UFUNCTION()
	void HandleButtonClicked();

	FLFPUnitEntry CachedEntry;

	UPROPERTY()
	TObjectPtr<ULFPUnitRegistryDataAsset> CachedRegistry;
};
