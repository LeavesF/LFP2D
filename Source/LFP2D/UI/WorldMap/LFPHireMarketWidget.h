#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Shop/LFPHireMarketTypes.h"
#include "LFPHireMarketWidget.generated.h"

class ULFPCardDataAsset;
class ULFPCardItemWidget;
class ULFPGameInstance;
class ULFPUnitReplacementWidget;
class UButton;
class UTextBlock;
class UImage;
class UVerticalBox;
class UWrapBox;
class ULFPSkillBase;
struct FLFPUnitRegistryEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHireMarketWidgetClosed);

UCLASS()
class LFP2D_API ULFPHireMarketWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化雇佣市场数据源和当前市场条目，并触发整页刷新。
	UFUNCTION(BlueprintCallable, Category = "HireMarket")
	void Setup(ULFPGameInstance* GI, FName InHireMarketID, const FLFPHireMarketDefinition& InHireMarketDefinition);

	UPROPERTY(BlueprintAssignable)
	FOnHireMarketWidgetClosed OnClosed;

protected:
	virtual void NativeOnInitialized() override;

private:
	// 刷新标题、金币、单位列表和当前选中单位详情。
	void RefreshHireMarketUI();
	// 重建左侧单位列表按钮。
	void RefreshUnitList();
	// 刷新当前选中单位的详情、购买状态和卡牌预览。
	void RefreshDetailPanel();
	// 清空当前单位的卡牌预览容器和缓存实例。
	void ClearSelectedUnitCardPreview();
	// 根据单位注册表条目重建只读卡牌预览。
	void RefreshSelectedUnitCardPreview(const FLFPUnitRegistryEntry& UnitDefinition);
	// 生成单位的属性说明文本。
	FString BuildUnitDescription(FName UnitTypeID) const;

	UFUNCTION() void OnUnitButtonClicked();
	UFUNCTION() void OnPurchaseClicked();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnReplacementComplete();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Gold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_UnitList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SelectedUnit;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedDescription;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedPrice;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PurchaseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Purchase;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_SelectedUnitCards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket")
	TSubclassOf<ULFPUnitReplacementWidget> ReplacementWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards")
	TSubclassOf<ULFPCardItemWidget> SelectedUnitCardItemWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards", meta = (ClampMin = "0"))
	int32 MaxSelectedUnitPreviewCards = 16;

	// 雇佣市场详情面板中单张预览卡的固定占位大小。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards")
	FVector2D SelectedUnitCardPreviewSize = FVector2D(320.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> PreviewFallbackMeleeAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> PreviewFallbackRangedAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> PreviewFallbackMagicAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> PreviewFallbackMeleeAttackClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> PreviewFallbackRangedAttackClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket|Cards|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> PreviewFallbackMagicAttackClass;

	UPROPERTY()
	TObjectPtr<ULFPUnitReplacementWidget> ReplacementWidget;

	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGameInstance;

	FName CachedHireMarketID = NAME_None;
	FLFPHireMarketDefinition CachedHireMarketDefinition;
	int32 SelectedUnitIndex = INDEX_NONE;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> UnitButtons;

	UPROPERTY()
	TArray<FLFPCardInstance> DisplayedSelectedUnitCards;

	TMap<UButton*, int32> ButtonToUnitIndexMap;
};
