#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "LFPShopWidget.generated.h"

class ULFPGameInstance;
class UButton;
class UTextBlock;
class UImage;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopWidgetClosed);

UCLASS()
class LFP2D_API ULFPShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void Setup(ULFPGameInstance* GI, FName InShopID, const FLFPShopDefinition& InShopDefinition);

	UPROPERTY(BlueprintAssignable)
	FOnShopWidgetClosed OnClosed;

protected:
	virtual void NativeOnInitialized() override;

private:
	void RefreshShopUI();
	void RefreshRelicList();
	void RefreshOwnedRelicList();
	void RefreshDetailPanel();
	FString BuildEffectDescription(const FLFPRelicDefinition& Definition) const;

	UFUNCTION() void OnRelicButtonClicked();
	UFUNCTION() void OnPurchaseClicked();
	UFUNCTION() void OnCloseClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Gold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_RelicList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_OwnedRelicList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SelectedRelic;

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

	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGameInstance;

	FName CachedShopID = NAME_None;
	FLFPShopDefinition CachedShopDefinition;
	int32 SelectedRelicIndex = INDEX_NONE;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> RelicButtons;

	TMap<UButton*, int32> ButtonToRelicIndexMap;
};
