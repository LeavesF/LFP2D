#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"

#include "LFP2D/Card/LFPBattleCardComponent.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

ULFPCardItemWidget::ULFPCardItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULFPCardItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CardButton)
	{
		CardButton->OnClicked.AddDynamic(this, &ULFPCardItemWidget::OnCardButtonClicked);
	}

	if (HighlightBorder)
	{
		HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPCardItemWidget::InitializeCardItem(const FLFPCardInstance& InCardInstance, ALFPTacticsPlayerController* InPC)
{
	CardInstance = InCardInstance;
	TacticsPC = InPC;

	const ULFPSkillBase* Skill = CardInstance.RuntimeSkill;
	if (!Skill)
	{
		return;
	}

	if (CardIcon)
	{
		CardIcon->SetBrushFromTexture(Skill->SkillIcon);
	}

	if (CardNameText)
	{
		CardNameText->SetText(Skill->SkillName);
	}

	if (CostText)
	{
		CostText->SetText(FText::AsNumber(Skill->ActionPointCost));
	}

	if (SourceUnitText)
	{
		if (CardInstance.SourceUnit != nullptr)
		{
			const FString UnitID = CardInstance.SourceUnit->UnitTypeID.ToString();
			SourceUnitText->SetText(FText::FromString(UnitID));
			SourceUnitText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SourceUnitText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULFPCardItemWidget::SetHighlighted(bool bHighlighted)
{
	if (HighlightBorder)
	{
		HighlightBorder->SetVisibility(bHighlighted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (CardIcon)
	{
		CardIcon->SetColorAndOpacity(bHighlighted ? HighlightedTint : NormalTint);
	}
}

void ULFPCardItemWidget::OnCardButtonClicked()
{
	if (CardInstance.IsValid())
	{
		OnCardClicked.Broadcast(CardInstance);
	}
}

void ULFPCardItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (CardInstance.IsValid())
	{
		OnCardHovered.Broadcast(CardInstance);
	}
}

void ULFPCardItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnCardUnhovered.Broadcast();
}
