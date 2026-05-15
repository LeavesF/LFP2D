#include "LFP2D/UI/Fighting/LFPPendingCardWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULFPPendingCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HidePendingCard();
}

void ULFPPendingCardWidget::ShowPendingCard(const FLFPCardInstance& InCard)
{
	PendingCard = InCard;
	bHasPendingCard = InCard.IsValid();

	if (CardIcon)
	{
		CardIcon->SetBrushFromTexture(InCard.Definition.Icon);
	}

	if (CardNameText)
	{
		CardNameText->SetText(InCard.Definition.DisplayName);
	}

	if (HintText)
	{
		HintText->SetText(FText::FromString(TEXT("请选择释放目标")));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void ULFPPendingCardWidget::HidePendingCard()
{
	bHasPendingCard = false;
	PendingCard = FLFPCardInstance();
	SetVisibility(ESlateVisibility::Collapsed);
}
