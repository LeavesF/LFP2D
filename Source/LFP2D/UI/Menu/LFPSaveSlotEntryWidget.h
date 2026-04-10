#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPSaveSlotEntryWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Single save slot entry widget.
 * Expects a UMG Blueprint with:
 *   - "SlotButton" (UButton) - the clickable area
 *   - "SlotText"  (UTextBlock, optional) - shows save info
 */
UCLASS()
class LFP2D_API ULFPSaveSlotEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set slot data
	UFUNCTION(BlueprintCallable, Category = "Save Slot")
	void SetSlotInfo(int32 InSlotIndex, const FText& InDisplayText);

protected:
	virtual void NativeConstruct() override;

	// Delegate fired when this slot is clicked
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveSlotClicked, int32, SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "Save Slot")
	FOnSaveSlotClicked OnSlotSelected;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

private:
	UFUNCTION() void OnSlotClicked();

	UPROPERTY()
	int32 SlotIndex = -1;
};
