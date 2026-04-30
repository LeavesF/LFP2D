#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/UI/LFPUnitSlotWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"

void ULFPDeploymentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnConfirmClicked);
	}
}

void ULFPDeploymentWidget::Setup(const TArray<FLFPUnitEntry>& PartyUnits, const TArray<FLFPUnitEntry>& ReserveUnits, ULFPUnitRegistryDataAsset* Registry)
{
	CachedPartyUnits = PartyUnits;
	CachedReserveUnits = ReserveUnits;
	UnitRegistry = Registry;
	SelectedPartyIndex = -1;
	SelectedReserveIndex = -1;

	// 清空并重建出战行
	ClearContainer(Box_Party, PartySlotWidgets);
	if (Box_Party)
	{
		CreateSlots(PartyUnits, Box_Party, PartySlotWidgets, true);
	}

	// 清空并重建备战行
	ClearContainer(Box_Reserve, ReserveSlotWidgets);
	if (Box_Reserve)
	{
		CreateSlots(ReserveUnits, Box_Reserve, ReserveSlotWidgets, false);
	}

	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(true);
	}
}

void ULFPDeploymentWidget::CreateSlots(const TArray<FLFPUnitEntry>& Units, UWrapBox* Container,
	TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets, bool bIsParty)
{
	if (!Container || !UnitSlotWidgetClass) return;

	OutWidgets.Empty();

	for (int32 i = 0; i < Units.Num(); i++)
	{
		if (!Units[i].IsValid()) continue;

		ULFPUnitSlotWidget* UnitSlot = CreateWidget<ULFPUnitSlotWidget>(this, UnitSlotWidgetClass);
		if (!UnitSlot) continue;

		UnitSlot->Setup(Units[i], UnitRegistry);
		if (bIsParty)
		{
			UnitSlot->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnPartySlotClicked);
		}
		else
		{
			UnitSlot->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnReserveSlotClicked);
		}
		Container->AddChildToWrapBox(UnitSlot);
		OutWidgets.Add(UnitSlot);
	}
}

void ULFPDeploymentWidget::ClearContainer(UWrapBox* Container, TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets)
{
	if (Container)
	{
		Container->ClearChildren();
	}
	OutWidgets.Empty();
}

// ============== 更新槽位 ==============

void ULFPDeploymentWidget::UpdatePartySlot(int32 PartyIndex, const FLFPUnitEntry& Entry)
{
	if (!CachedPartyUnits.IsValidIndex(PartyIndex)) return;

	CachedPartyUnits[PartyIndex] = Entry;

	if (PartySlotWidgets.IsValidIndex(PartyIndex))
	{
		if (Entry.IsValid())
		{
			PartySlotWidgets[PartyIndex]->Setup(Entry, UnitRegistry);
			if (SelectedPartyIndex == PartyIndex)
			{
				PartySlotWidgets[PartyIndex]->SetSelected(true);
			}
		}
		else
		{
			PartySlotWidgets[PartyIndex]->Clear();
		}
	}
}

void ULFPDeploymentWidget::UpdateReserveSlot(int32 ReserveIndex, const FLFPUnitEntry& Entry)
{
	if (!CachedReserveUnits.IsValidIndex(ReserveIndex)) return;

	CachedReserveUnits[ReserveIndex] = Entry;

	if (ReserveSlotWidgets.IsValidIndex(ReserveIndex))
	{
		if (Entry.IsValid())
		{
			ReserveSlotWidgets[ReserveIndex]->Setup(Entry, UnitRegistry);
			if (SelectedReserveIndex == ReserveIndex)
			{
				ReserveSlotWidgets[ReserveIndex]->SetSelected(true);
			}
		}
		else
		{
			ReserveSlotWidgets[ReserveIndex]->Clear();
		}
	}
}

// ============== 选中高亮 ==============

void ULFPDeploymentWidget::MarkPartyUnitSelected(int32 PartyIndex, bool bSelected)
{
	if (PartySlotWidgets.IsValidIndex(PartyIndex))
	{
		PartySlotWidgets[PartyIndex]->SetSelected(bSelected);
	}

	if (bSelected)
	{
		SelectedPartyIndex = PartyIndex;
		// 取消备战选中
		if (SelectedReserveIndex >= 0 && ReserveSlotWidgets.IsValidIndex(SelectedReserveIndex))
		{
			ReserveSlotWidgets[SelectedReserveIndex]->SetSelected(false);
			SelectedReserveIndex = -1;
		}
	}
	else if (SelectedPartyIndex == PartyIndex)
	{
		SelectedPartyIndex = -1;
	}
}

void ULFPDeploymentWidget::MarkReserveUnitSelected(int32 ReserveIndex, bool bSelected)
{
	if (ReserveSlotWidgets.IsValidIndex(ReserveIndex))
	{
		ReserveSlotWidgets[ReserveIndex]->SetSelected(bSelected);
	}

	if (bSelected)
	{
		SelectedReserveIndex = ReserveIndex;
		if (SelectedPartyIndex >= 0 && PartySlotWidgets.IsValidIndex(SelectedPartyIndex))
		{
			PartySlotWidgets[SelectedPartyIndex]->SetSelected(false);
			SelectedPartyIndex = -1;
		}
	}
	else if (SelectedReserveIndex == ReserveIndex)
	{
		SelectedReserveIndex = -1;
	}
}

void ULFPDeploymentWidget::ClearAllSelections()
{
	if (SelectedPartyIndex >= 0 && PartySlotWidgets.IsValidIndex(SelectedPartyIndex))
	{
		PartySlotWidgets[SelectedPartyIndex]->SetSelected(false);
		SelectedPartyIndex = -1;
	}
	if (SelectedReserveIndex >= 0 && ReserveSlotWidgets.IsValidIndex(SelectedReserveIndex))
	{
		ReserveSlotWidgets[SelectedReserveIndex]->SetSelected(false);
		SelectedReserveIndex = -1;
	}
}

// ============== 按钮回调 ==============

void ULFPDeploymentWidget::OnPartySlotClicked(ULFPUnitSlotWidget* ClickedSlot)
{
	int32 Index = PartySlotWidgets.Find(ClickedSlot);
	if (Index != INDEX_NONE)
	{
		OnPartyUnitClicked.Broadcast(Index);
	}
}

void ULFPDeploymentWidget::OnReserveSlotClicked(ULFPUnitSlotWidget* ClickedSlot)
{
	int32 Index = ReserveSlotWidgets.Find(ClickedSlot);
	if (Index != INDEX_NONE)
	{
		OnReserveUnitClicked.Broadcast(Index);
	}
}

void ULFPDeploymentWidget::OnConfirmClicked()
{
	OnConfirmPressed.Broadcast();
}
