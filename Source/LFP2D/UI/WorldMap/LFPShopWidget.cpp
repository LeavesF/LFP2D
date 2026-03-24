#include "LFP2D/UI/WorldMap/LFPShopWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

void ULFPShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Purchase) Button_Purchase->OnClicked.AddDynamic(this, &ULFPShopWidget::OnPurchaseClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPShopWidget::OnCloseClicked);
}

void ULFPShopWidget::Setup(ULFPGameInstance* GI, FName InShopID, const FLFPShopDefinition& InShopDefinition)
{
	CachedGameInstance = GI;
	CachedShopID = InShopID;
	CachedShopDefinition = InShopDefinition;
	SelectedRelicIndex = CachedShopDefinition.RelicList.Num() > 0 ? 0 : INDEX_NONE;
	RefreshShopUI();
}

void ULFPShopWidget::RefreshShopUI()
{
	if (Text_Title)
	{
		Text_Title->SetText(CachedShopDefinition.DisplayName.IsEmpty() ? FText::FromString(TEXT("遗物商店")) : CachedShopDefinition.DisplayName);
	}

	if (Text_Gold && CachedGameInstance)
	{
		Text_Gold->SetText(FText::FromString(FString::Printf(TEXT("金币: %d"), CachedGameInstance->GetGold())));
	}

	RefreshRelicList();
	RefreshDetailPanel();
}

void ULFPShopWidget::RefreshRelicList()
{
	RelicButtons.Empty();
	ButtonToRelicIndexMap.Empty();

	if (Box_RelicList)
	{
		Box_RelicList->ClearChildren();
	}

	if (!CachedGameInstance || !Box_RelicList)
	{
		return;
	}

	for (int32 i = 0; i < CachedShopDefinition.RelicList.Num(); ++i)
	{
		const FLFPShopRelicEntry& Entry = CachedShopDefinition.RelicList[i];
		FLFPRelicDefinition RelicDefinition;
		if (!CachedGameInstance->FindRelicDefinition(Entry.RelicID, RelicDefinition))
		{
			continue;
		}

		UButton* RelicButton = NewObject<UButton>(this);
		if (!RelicButton)
		{
			continue;
		}

		UTextBlock* Label = NewObject<UTextBlock>(RelicButton);
		if (Label)
		{
			FString Suffix;
			if (CachedGameInstance->HasRelic(Entry.RelicID))
			{
				Suffix = TEXT(" [已拥有]");
			}
			else if (!CachedGameInstance->CanAffordGold(Entry.Price))
			{
				Suffix = TEXT(" [金币不足]");
			}

			Label->SetText(FText::FromString(FString::Printf(TEXT("%s - %d%s"), *RelicDefinition.DisplayName.ToString(), Entry.Price, *Suffix)));
			RelicButton->AddChild(Label);
		}

		UVerticalBoxSlot* ItemSlot = Box_RelicList->AddChildToVerticalBox(RelicButton);
		if (ItemSlot)
		{
			ItemSlot->SetPadding(FMargin(4.f));
		}

		RelicButton->OnClicked.AddDynamic(this, &ULFPShopWidget::OnRelicButtonClicked);
		RelicButtons.Add(RelicButton);
		ButtonToRelicIndexMap.Add(RelicButton, i);
	}
}

void ULFPShopWidget::RefreshDetailPanel()
{
	if (!CachedGameInstance)
	{
		return;
	}

	if (!CachedShopDefinition.RelicList.IsValidIndex(SelectedRelicIndex))
	{
		if (Text_SelectedName) Text_SelectedName->SetText(FText::FromString(TEXT("未选择遗物")));
		if (Text_SelectedDescription) Text_SelectedDescription->SetText(FText::GetEmpty());
		if (Text_SelectedPrice) Text_SelectedPrice->SetText(FText::GetEmpty());
		if (Text_Status) Text_Status->SetText(FText::GetEmpty());
		if (Button_Purchase) Button_Purchase->SetIsEnabled(false);
		if (Image_SelectedRelic) Image_SelectedRelic->SetBrushFromTexture(nullptr);
		return;
	}

	const FLFPShopRelicEntry& Entry = CachedShopDefinition.RelicList[SelectedRelicIndex];
	FLFPRelicDefinition Definition;
	if (!CachedGameInstance->FindRelicDefinition(Entry.RelicID, Definition))
	{
		if (Text_Status) Text_Status->SetText(FText::FromString(TEXT("遗物定义缺失")));
		if (Button_Purchase) Button_Purchase->SetIsEnabled(false);
		return;
	}

	if (Image_SelectedRelic)
	{
		Image_SelectedRelic->SetBrushFromTexture(Definition.Icon);
	}
	if (Text_SelectedName)
	{
		Text_SelectedName->SetText(Definition.DisplayName);
	}
	if (Text_SelectedDescription)
	{
		Text_SelectedDescription->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *Definition.Description.ToString(), *BuildEffectDescription(Definition))));
	}
	if (Text_SelectedPrice)
	{
		Text_SelectedPrice->SetText(FText::FromString(FString::Printf(TEXT("价格: %d"), Entry.Price)));
	}

	bool bOwned = CachedGameInstance->HasRelic(Entry.RelicID);
	bool bCanAfford = CachedGameInstance->CanAffordGold(Entry.Price);
	if (Button_Purchase)
	{
		Button_Purchase->SetIsEnabled(!bOwned && bCanAfford);
	}
	if (Text_Status)
	{
		if (bOwned)
		{
			Text_Status->SetText(FText::FromString(TEXT("已拥有")));
		}
		else if (!bCanAfford)
		{
			Text_Status->SetText(FText::FromString(TEXT("金币不足")));
		}
		else
		{
			Text_Status->SetText(FText::FromString(TEXT("可购买")));
		}
	}
}

FString ULFPShopWidget::BuildEffectDescription(const FLFPRelicDefinition& Definition) const
{
	TArray<FString> Parts;
	for (const FLFPRelicEffectEntry& Effect : Definition.Effects)
	{
		switch (Effect.EffectType)
		{
		case ELFPRelicEffectType::RET_MaxHealthFlat:
			Parts.Add(FString::Printf(TEXT("最大生命 %+d"), Effect.Value));
			break;
		case ELFPRelicEffectType::RET_AttackFlat:
			Parts.Add(FString::Printf(TEXT("攻击力 %+d"), Effect.Value));
			break;
		case ELFPRelicEffectType::RET_DefenseFlat:
			Parts.Add(FString::Printf(TEXT("防御力 %+d"), Effect.Value));
			break;
		case ELFPRelicEffectType::RET_SpeedFlat:
			Parts.Add(FString::Printf(TEXT("速度 %+d"), Effect.Value));
			break;
		default:
			break;
		}
	}

	return Parts.Num() > 0 ? FString::Join(Parts, TEXT("\n")) : TEXT("无效果");
}

void ULFPShopWidget::OnRelicButtonClicked()
{
	for (const auto& Pair : ButtonToRelicIndexMap)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			SelectedRelicIndex = Pair.Value;
			RefreshDetailPanel();
			return;
		}
	}
}

void ULFPShopWidget::OnPurchaseClicked()
{
	if (!CachedGameInstance || !CachedShopDefinition.RelicList.IsValidIndex(SelectedRelicIndex))
	{
		return;
	}

	const FLFPShopRelicEntry& Entry = CachedShopDefinition.RelicList[SelectedRelicIndex];
	if (CachedGameInstance->TryPurchaseRelic(Entry.RelicID, Entry.Price))
	{
		RefreshShopUI();
	}
}

void ULFPShopWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	OnClosed.Broadcast();
}
