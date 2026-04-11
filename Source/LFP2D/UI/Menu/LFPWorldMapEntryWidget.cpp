#include "LFP2D/UI/Menu/LFPWorldMapEntryWidget.h"
#include "LFP2D/UI/Menu/LFPWorldMapListItem.h"
#include "Components/TextBlock.h"

void ULFPWorldMapEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	ULFPWorldMapListItem* Item = Cast<ULFPWorldMapListItem>(ListItemObject);
	if (Text_MapName)
	{
		Text_MapName->SetText(FText::FromString(Item ? Item->MapName : FString()));
	}
}
