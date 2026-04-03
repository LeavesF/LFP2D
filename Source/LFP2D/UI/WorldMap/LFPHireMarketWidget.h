#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Shop/LFPHireMarketTypes.h"
#include "LFPHireMarketWidget.generated.h"

class ULFPGameInstance;
class ULFPUnitReplacementWidget;
class UButton;
class UTextBlock;
class UImage;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHireMarketWidgetClosed);

UCLASS()
class LFP2D_API ULFPHireMarketWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HireMarket")
	void Setup(ULFPGameInstance* GI, FName InHireMarketID, const FLFPHireMarketDefinition& InHireMarketDefinition);

	UPROPERTY(BlueprintAssignable)
	FOnHireMarketWidgetClosed OnClosed;

protected:
	virtual void NativeOnInitialized() override;

private:
	void RefreshHireMarketUI();
	void RefreshUnitList();
	void RefreshDetailPanel();
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
	TObjectPtr<UButton> Button_Purchase;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket")
	TSubclassOf<ULFPUnitReplacementWidget> ReplacementWidgetClass;

	UPROPERTY()
	TObjectPtr<ULFPUnitReplacementWidget> ReplacementWidget;

	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGameInstance;

	FName CachedHireMarketID = NAME_None;
	FLFPHireMarketDefinition CachedHireMarketDefinition;
	int32 SelectedUnitIndex = INDEX_NONE;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> UnitButtons;

	TMap<UButton*, int32> ButtonToUnitIndexMap;
};
