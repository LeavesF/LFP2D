#include "LFP2D/UI/Fighting/LFPDeploymentDeckPreviewWidget.h"

#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

void ULFPDeploymentDeckPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPDeploymentDeckPreviewWidget::RefreshDeckPreview(
	const TArray<FLFPCardInstance>& Cards,
	ALFPTacticsPlayerController* PC)
{
	if (CardContainer)
	{
		CardContainer->ClearChildren();
	}

	if (CardCountText)
	{
		CardCountText->SetText(FText::AsNumber(Cards.Num()));
	}

	if (!CardContainer || !CardItemWidgetClass)
	{
		return;
	}

	for (const FLFPCardInstance& Card : Cards)
	{
		if (!Card.IsValid())
		{
			continue;
		}

		ULFPCardItemWidget* CardItem = CreateWidget<ULFPCardItemWidget>(this, CardItemWidgetClass);
		if (!CardItem)
		{
			continue;
		}

		CardItem->InitializeCardItem(Card, PC);
		CardContainer->AddChildToWrapBox(CardItem);
	}
}

void ULFPDeploymentDeckPreviewWidget::Show()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void ULFPDeploymentDeckPreviewWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPDeploymentDeckPreviewWidget::ToggleVisibility()
{
	if (IsVisible())
	{
		Hide();
	}
	else
	{
		Show();
	}
}
