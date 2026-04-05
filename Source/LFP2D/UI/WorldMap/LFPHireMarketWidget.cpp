#include "LFP2D/UI/WorldMap/LFPHireMarketWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/UI/WorldMap/LFPUnitReplacementWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void ULFPHireMarketWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Purchase) Button_Purchase->OnClicked.AddDynamic(this, &ULFPHireMarketWidget::OnPurchaseClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPHireMarketWidget::OnCloseClicked);
}

void ULFPHireMarketWidget::Setup(ULFPGameInstance* GI, FName InHireMarketID, const FLFPHireMarketDefinition& InHireMarketDefinition)
{
	CachedGameInstance = GI;
	CachedHireMarketID = InHireMarketID;
	CachedHireMarketDefinition = InHireMarketDefinition;
	SelectedUnitIndex = CachedHireMarketDefinition.UnitList.Num() > 0 ? 0 : INDEX_NONE;
	RefreshHireMarketUI();
}

void ULFPHireMarketWidget::RefreshHireMarketUI()
{
	if (Text_Title)
	{
		const FText BaseTitle = CachedHireMarketDefinition.DisplayName.IsEmpty() ? FText::FromString(TEXT("雇佣市场")) : CachedHireMarketDefinition.DisplayName;
		Text_Title->SetText(BaseTitle);
	}

	if (Text_Gold && CachedGameInstance)
	{
		Text_Gold->SetText(FText::FromString(FString::Printf(TEXT("金币: %d"), CachedGameInstance->GetGold())));
	}

	RefreshUnitList();
	RefreshDetailPanel();
}

void ULFPHireMarketWidget::RefreshUnitList()
{
	UnitButtons.Empty();
	ButtonToUnitIndexMap.Empty();

	if (Box_UnitList)
	{
		Box_UnitList->ClearChildren();
	}

	if (!CachedGameInstance || !CachedGameInstance->UnitRegistry || !Box_UnitList)
	{
		return;
	}

	for (int32 i = 0; i < CachedHireMarketDefinition.UnitList.Num(); ++i)
	{
		const FLFPHireMarketUnitEntry& Entry = CachedHireMarketDefinition.UnitList[i];
		FLFPUnitRegistryEntry UnitDefinition;
		if (!CachedGameInstance->UnitRegistry->FindEntry(Entry.UnitTypeID, UnitDefinition))
		{
			continue;
		}

		const bool bPurchased = CachedGameInstance->HasPurchasedHireMarketUnit(CachedHireMarketID, Entry.UnitTypeID);

		UButton* UnitButton = NewObject<UButton>(this);
		if (!UnitButton)
		{
			continue;
		}

		UTextBlock* Label = NewObject<UTextBlock>(UnitButton);
		if (Label)
		{
			FString Suffix;
			if (bPurchased)
			{
				Suffix = TEXT(" [已购买]");
			}
			else if (!CachedGameInstance->CanAffordGold(Entry.Price))
			{
				Suffix = TEXT(" [金币不足]");
			}

			Label->SetText(FText::FromString(FString::Printf(TEXT("%s - %d%s"), *UnitDefinition.DisplayName.ToString(), Entry.Price, *Suffix)));
			UnitButton->AddChild(Label);
		}

		UnitButton->SetIsEnabled(!bPurchased);

		UVerticalBoxSlot* ItemSlot = Box_UnitList->AddChildToVerticalBox(UnitButton);
		if (ItemSlot)
		{
			ItemSlot->SetPadding(FMargin(4.f));
		}

		UnitButton->OnClicked.AddDynamic(this, &ULFPHireMarketWidget::OnUnitButtonClicked);
		UnitButtons.Add(UnitButton);
		ButtonToUnitIndexMap.Add(UnitButton, i);
	}
}

void ULFPHireMarketWidget::RefreshDetailPanel()
{
	if (!CachedGameInstance || !CachedGameInstance->UnitRegistry)
	{
		return;
	}

	if (!CachedHireMarketDefinition.UnitList.IsValidIndex(SelectedUnitIndex))
	{
		if (Text_SelectedName) Text_SelectedName->SetText(FText::FromString(TEXT("未选择单位")));
		if (Text_SelectedDescription) Text_SelectedDescription->SetText(FText::GetEmpty());
		if (Text_SelectedPrice) Text_SelectedPrice->SetText(FText::GetEmpty());
		if (Text_Status) Text_Status->SetText(FText::GetEmpty());
		if (Button_Purchase) Button_Purchase->SetIsEnabled(false);
		if (Image_SelectedUnit) Image_SelectedUnit->SetBrushFromTexture(nullptr);
		return;
	}

	const FLFPHireMarketUnitEntry& Entry = CachedHireMarketDefinition.UnitList[SelectedUnitIndex];
	FLFPUnitRegistryEntry UnitDefinition;
	if (!CachedGameInstance->UnitRegistry->FindEntry(Entry.UnitTypeID, UnitDefinition))
	{
		if (Text_Status) Text_Status->SetText(FText::FromString(TEXT("单位定义缺失")));
		if (Button_Purchase) Button_Purchase->SetIsEnabled(false);
		return;
	}

	const bool bPurchased = CachedGameInstance->HasPurchasedHireMarketUnit(CachedHireMarketID, Entry.UnitTypeID);

	if (Image_SelectedUnit)
	{
		Image_SelectedUnit->SetBrushFromTexture(UnitDefinition.Icon);
	}
	if (Text_SelectedName)
	{
		Text_SelectedName->SetText(UnitDefinition.DisplayName);
	}
	if (Text_SelectedDescription)
	{
		Text_SelectedDescription->SetText(FText::FromString(BuildUnitDescription(Entry.UnitTypeID)));
	}
	if (Text_SelectedPrice)
	{
		Text_SelectedPrice->SetText(FText::FromString(FString::Printf(TEXT("价格: %d"), Entry.Price)));
	}

	const bool bCanAfford = CachedGameInstance->CanAffordGold(Entry.Price);
	const bool bCanPurchase = !bPurchased && bCanAfford;
	if (Button_Purchase)
	{
		Button_Purchase->SetIsEnabled(bCanPurchase);
	}
	if (Text_PurchaseButton)
	{
		if (bPurchased)
		{
			Text_PurchaseButton->SetText(FText::FromString(TEXT("已购买")));
		}
		else if (!bCanAfford)
		{
			Text_PurchaseButton->SetText(FText::FromString(TEXT("金币不足")));
		}
		else
		{
			Text_PurchaseButton->SetText(FText::FromString(TEXT("雇佣")));
		}
	}
	if (Text_Status)
	{
		if (bPurchased)
		{
			Text_Status->SetText(FText::FromString(TEXT("该单位已购买，当前市场不能重复雇佣")));
		}
		else if (!bCanAfford)
		{
			Text_Status->SetText(FText::FromString(TEXT("金币不足")));
		}
		else if (CachedGameInstance->IsPartyFull() && CachedGameInstance->IsReserveFull())
		{
			Text_Status->SetText(FText::FromString(TEXT("队伍与备战营已满，购买后需要替换现有单位")));
		}
		else if (CachedGameInstance->IsPartyFull())
		{
			Text_Status->SetText(FText::FromString(TEXT("出战队伍已满，购买后会进入备战营")));
		}
		else
		{
			Text_Status->SetText(FText::FromString(TEXT("可雇佣")));
		}
	}
}

FString ULFPHireMarketWidget::BuildUnitDescription(FName UnitTypeID) const
{
	if (!CachedGameInstance || !CachedGameInstance->UnitRegistry)
	{
		return TEXT("");
	}

	FLFPUnitRegistryEntry UnitDefinition;
	if (!CachedGameInstance->UnitRegistry->FindEntry(UnitTypeID, UnitDefinition))
	{
		return TEXT("单位定义缺失");
	}

	const FString RaceName = StaticEnum<ELFPUnitRace>()->GetDisplayNameTextByValue(static_cast<int64>(UnitDefinition.Race)).ToString();
	return FString::Printf(
		TEXT("阶级: T%d\n种族: %s\n攻击: %d\n生命: %d\n移动: %d\n速度: %d\n行动次数: %d\n攻击次数: %d\n物理格挡: %d\n法术防御: %d\n重量: %d"),
		UnitDefinition.Tier,
		*RaceName,
		UnitDefinition.BaseStats.Attack,
		UnitDefinition.BaseStats.MaxHealth,
		UnitDefinition.BaseStats.MaxMovePoints,
		UnitDefinition.BaseStats.Speed,
		UnitDefinition.AdvancedStats.ActionCount,
		UnitDefinition.AdvancedStats.AttackCount,
		UnitDefinition.AdvancedStats.PhysicalBlock,
		UnitDefinition.AdvancedStats.SpellDefense,
		UnitDefinition.AdvancedStats.Weight
	);
}

void ULFPHireMarketWidget::OnUnitButtonClicked()
{
	for (const auto& Pair : ButtonToUnitIndexMap)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			SelectedUnitIndex = Pair.Value;
			RefreshDetailPanel();
			return;
		}
	}
}

void ULFPHireMarketWidget::OnPurchaseClicked()
{
	if (!CachedGameInstance || !CachedHireMarketDefinition.UnitList.IsValidIndex(SelectedUnitIndex))
	{
		return;
	}

	const FLFPHireMarketUnitEntry& Entry = CachedHireMarketDefinition.UnitList[SelectedUnitIndex];
	FLFPUnitEntry NewUnit;
	if (!CachedGameInstance->TryPurchaseHireMarketUnit(CachedHireMarketID, Entry.UnitTypeID, Entry.Price, NewUnit))
	{
		RefreshHireMarketUI();
		return;
	}

	if (CachedGameInstance->TryAddUnit(NewUnit))
	{
		RefreshHireMarketUI();
		return;
	}

	if (ReplacementWidgetClass)
	{
		ReplacementWidget = CreateWidget<ULFPUnitReplacementWidget>(GetOwningPlayer(), ReplacementWidgetClass);
		if (ReplacementWidget)
		{
			ReplacementWidget->OnReplacementComplete.AddDynamic(this, &ULFPHireMarketWidget::OnReplacementComplete);
			ReplacementWidget->AddToViewport(10);
			ReplacementWidget->Setup(NewUnit, CachedGameInstance->PartyUnits, CachedGameInstance->ReserveUnits, CachedGameInstance->UnitRegistry);
		}
	}
}

void ULFPHireMarketWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	OnClosed.Broadcast();
}

void ULFPHireMarketWidget::OnReplacementComplete()
{
	if (ReplacementWidget)
	{
		ReplacementWidget->OnReplacementComplete.RemoveDynamic(this, &ULFPHireMarketWidget::OnReplacementComplete);
		ReplacementWidget->RemoveFromParent();
		ReplacementWidget = nullptr;
	}

	RefreshHireMarketUI();
}
