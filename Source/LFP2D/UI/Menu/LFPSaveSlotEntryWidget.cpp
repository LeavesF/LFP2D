#include "LFP2D/UI/Menu/LFPSaveSlotEntryWidget.h"
#include "LFP2D/UI/Menu/LFPSaveSlotListItem.h"
#include "Components/TextBlock.h"

void ULFPSaveSlotEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	if (!Text_SlotInfo) return;

	ULFPSaveSlotListItem* Item = Cast<ULFPSaveSlotListItem>(ListItemObject);
	if (!Item)
	{
		Text_SlotInfo->SetText(FText::GetEmpty());
		return;
	}

	const FLFPSaveSlotInfo& Info = Item->SlotInfo;
	FString DisplayText;
	if (Info.bIsValid)
	{
		DisplayText = FString::Printf(TEXT("Slot %d: %s\n%s | Turn %d\n%s"),
			Info.SlotIndex,
			*Info.SaveName,
			*Info.WorldMapName,
			Info.CurrentTurn,
			*Info.Timestamp.ToString());
	}
	else
	{
		DisplayText = FString::Printf(TEXT("Slot %d: 空"), Info.SlotIndex);
	}

	Text_SlotInfo->SetText(FText::FromString(DisplayText));
}
