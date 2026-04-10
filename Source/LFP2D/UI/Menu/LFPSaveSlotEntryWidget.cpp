#include "LFP2D/UI/Menu/LFPSaveSlotEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULFPSaveSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &ULFPSaveSlotEntryWidget::OnSlotClicked);
	}
}

void ULFPSaveSlotEntryWidget::SetSlotInfo(int32 InSlotIndex, const FText& InDisplayText)
{
	SlotIndex = InSlotIndex;
	if (SlotText)
	{
		SlotText->SetText(InDisplayText);
	}
}

void ULFPSaveSlotEntryWidget::OnSlotClicked()
{
	OnSlotSelected.Broadcast(SlotIndex);
}
