#include "LFP2D/UI/LFPUnitSlotWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/Button.h"
#include "Components/Image.h"

void ULFPUnitSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Unit)
	{
		Button_Unit->OnClicked.AddDynamic(this, &ULFPUnitSlotWidget::HandleButtonClicked);
	}
}

void ULFPUnitSlotWidget::Setup(const FLFPUnitEntry& Entry, ULFPUnitRegistryDataAsset* Registry)
{
	CachedEntry = Entry;
	CachedRegistry = Registry;

	if (Button_Unit)
	{
		Button_Unit->SetVisibility(ESlateVisibility::Visible);
		Button_Unit->SetIsEnabled(true);
	}

	if (Image_Unit && Registry)
	{
		FLFPUnitRegistryEntry RegEntry;
		if (Registry->FindEntry(Entry.TypeID, RegEntry) && RegEntry.Icon)
		{
			Image_Unit->SetBrushFromTexture(RegEntry.Icon);
			Image_Unit->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	SetSelected(false);
}

void ULFPUnitSlotWidget::Clear()
{
	CachedEntry = FLFPUnitEntry();
	CachedRegistry = nullptr;

	if (Button_Unit)
	{
		Button_Unit->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPUnitSlotWidget::SetSelected(bool bSelected)
{
	if (Image_Unit)
	{
		Image_Unit->SetColorAndOpacity(bSelected
			? FLinearColor(1.0f, 0.9f, 0.3f, 1.0f)   // 金色高亮
			: FLinearColor::White);
	}
}

void ULFPUnitSlotWidget::HandleButtonClicked()
{
	OnClicked.Broadcast(this);
}
