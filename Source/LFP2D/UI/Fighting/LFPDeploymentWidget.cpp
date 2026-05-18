#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/UI/LFPUnitSlotWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/UI/Fighting/LFPDeploymentDeckPreviewWidget.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"

void ULFPDeploymentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnConfirmClicked);
	}

	if (Button_ToggleDeckPreview)
	{
		Button_ToggleDeckPreview->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnToggleDeckPreviewClicked);
	}

	if (DeckPreviewWidget)
	{
		DeckPreviewWidget->Hide();
	}
}

void ULFPDeploymentWidget::Setup(const TArray<FLFPUnitEntry>& PartyUnits, const TArray<FLFPUnitEntry>& ReserveUnits, ULFPUnitRegistryDataAsset* Registry)
{
	CachedUnits.Empty();
	CachedDeploymentStates.Empty();
	UnitRegistry = Registry;
	SelectedUnitIndex = -1;

	for (const FLFPUnitEntry& Unit : PartyUnits)
	{
		if (Unit.IsValid())
		{
			CachedUnits.Add(Unit);
			CachedDeploymentStates.Add(true);
		}
	}

	for (const FLFPUnitEntry& Unit : ReserveUnits)
	{
		if (Unit.IsValid())
		{
			CachedUnits.Add(Unit);
			CachedDeploymentStates.Add(false);
		}
	}

	UWrapBox* UnitsContainer = GetUnitsContainer();
	ClearContainer(UnitsContainer, UnitSlotWidgets);
	if (UnitsContainer)
	{
		CreateUnitSlots(UnitsContainer);
	}

	if (Box_Party && Box_Party != UnitsContainer)
	{
		Box_Party->ClearChildren();
		Box_Party->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Box_Reserve && Box_Reserve != UnitsContainer)
	{
		Box_Reserve->ClearChildren();
		Box_Reserve->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(true);
	}

	if (DeckPreviewWidget)
	{
		DeckPreviewWidget->Hide();
	}
}

void ULFPDeploymentWidget::SetUnitDeployed(int32 UnitIndex, bool bIsDeployed)
{
	if (CachedDeploymentStates.IsValidIndex(UnitIndex))
	{
		CachedDeploymentStates[UnitIndex] = bIsDeployed;
	}

	if (UnitSlotWidgets.IsValidIndex(UnitIndex) && UnitSlotWidgets[UnitIndex])
	{
		UnitSlotWidgets[UnitIndex]->SetDeployed(bIsDeployed);
	}
}

void ULFPDeploymentWidget::ClearAllSelections()
{
	if (SelectedUnitIndex >= 0 && UnitSlotWidgets.IsValidIndex(SelectedUnitIndex) && UnitSlotWidgets[SelectedUnitIndex])
	{
		UnitSlotWidgets[SelectedUnitIndex]->SetSelected(false);
	}
	SelectedUnitIndex = -1;
}

void ULFPDeploymentWidget::RefreshDeckPreview(const TArray<FLFPCardInstance>& Cards, ALFPTacticsPlayerController* PC)
{
	if (DeckPreviewWidget)
	{
		DeckPreviewWidget->RefreshDeckPreview(Cards, PC);
	}
}

UWrapBox* ULFPDeploymentWidget::GetUnitsContainer() const
{
	return Box_Units ? Box_Units.Get() : Box_Party.Get();
}

void ULFPDeploymentWidget::CreateUnitSlots(UWrapBox* Container)
{
	if (!Container || !UnitSlotWidgetClass)
	{
		return;
	}

	UnitSlotWidgets.Empty();
	for (int32 i = 0; i < CachedUnits.Num(); ++i)
	{
		ULFPUnitSlotWidget* UnitSlot = CreateWidget<ULFPUnitSlotWidget>(this, UnitSlotWidgetClass);
		if (!UnitSlot)
		{
			continue;
		}

		UnitSlot->Setup(CachedUnits[i], UnitRegistry);
		UnitSlot->SetDeployed(CachedDeploymentStates.IsValidIndex(i) && CachedDeploymentStates[i]);
		UnitSlot->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnUnitSlotClicked);
		Container->AddChildToWrapBox(UnitSlot);
		UnitSlotWidgets.Add(UnitSlot);
	}
}

void ULFPDeploymentWidget::ClearContainer(UWrapBox* Container, TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets)
{
	if (Container)
	{
		Container->ClearChildren();
		Container->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	OutWidgets.Empty();
}

void ULFPDeploymentWidget::OnUnitSlotClicked(ULFPUnitSlotWidget* ClickedSlot)
{
	const int32 Index = UnitSlotWidgets.Find(ClickedSlot);
	if (Index != INDEX_NONE)
	{
		OnUnitClicked.Broadcast(Index);
	}
}

void ULFPDeploymentWidget::OnToggleDeckPreviewClicked()
{
	if (DeckPreviewWidget)
	{
		DeckPreviewWidget->ToggleVisibility();
	}
}

void ULFPDeploymentWidget::OnConfirmClicked()
{
	OnConfirmPressed.Broadcast();
}
