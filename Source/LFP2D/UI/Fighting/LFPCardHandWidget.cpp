#include "LFP2D/UI/Fighting/LFPCardHandWidget.h"

#include "LFP2D/Card/LFPBattleCardComponent.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

ULFPCardHandWidget::ULFPCardHandWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULFPCardHandWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULFPCardHandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HiddenDraggedCardInstanceID != INDEX_NONE && (!TacticsPC || !TacticsPC->IsCardDragActive()))
	{
		RestoreHiddenDraggedCard();
	}
}

void ULFPCardHandWidget::InitializeFromBattleCardComponent(ULFPBattleCardComponent* InCardComponent, ALFPTacticsPlayerController* InPC)
{
	CardComponent = InCardComponent;
	TacticsPC = InPC;
	RefreshHandDisplay();
}

void ULFPCardHandWidget::RefreshHandDisplay()
{
	if (!CardContainer)
	{
		return;
	}

	CardContainer->ClearChildren();
	HandCardWidgets.Empty();

	if (TacticsPC && !TacticsPC->IsCardDragActive())
	{
		TacticsPC->ClearCardUsableUnitPreview();
	}

	if (!CardComponent || !TacticsPC)
	{
		return;
	}

	const TArray<FLFPCardInstance> HandCards = CardComponent->GetHand();
	if (HandCards.IsEmpty() || !CardItemWidgetClass)
	{
		return;
	}

	for (const FLFPCardInstance& Card : HandCards)
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

		CardItem->InitializeCardItem(Card, TacticsPC);
		CardItem->OnCardClicked.AddDynamic(this, &ULFPCardHandWidget::OnCardClickedInHand);
		CardItem->OnCardHovered.AddDynamic(this, &ULFPCardHandWidget::OnCardHoveredInHand);
		CardItem->OnCardUnhovered.AddDynamic(this, &ULFPCardHandWidget::OnCardUnhoveredInHand);
		CardItem->OnCardDragStarted.AddDynamic(this, &ULFPCardHandWidget::OnCardDragStartedInHand);
		CardItem->OnCardDragEnded.AddDynamic(this, &ULFPCardHandWidget::OnCardDragEndedInHand);

		CardContainer->AddChildToWrapBox(CardItem);
		HandCardWidgets.Add(CardItem);
	}

	if (HandCountText)
	{
		HandCountText->SetText(FText::Format(FText::FromString(TEXT("{0}/{1}")),
			FText::AsNumber(HandCards.Num()),
			FText::AsNumber(CardComponent->GetMaxHandSize())));
	}

	if (DrawPileCountText)
	{
		DrawPileCountText->SetText(FText::AsNumber(CardComponent->GetDrawPile().Num()));
	}

	if (DiscardPileCountText)
	{
		DiscardPileCountText->SetText(FText::AsNumber(CardComponent->GetDiscardPile().Num()));
	}

	if (ExhaustPileCountText)
	{
		ExhaustPileCountText->SetText(FText::AsNumber(CardComponent->GetExhaustPile().Num()));
	}

	ApplyUnitPlayablePopups();
}

void ULFPCardHandWidget::Show()
{
	SetVisibility(ESlateVisibility::Visible);
	RefreshHandDisplay();
}

void ULFPCardHandWidget::Hide()
{
	ResetAllCardPopups();
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPCardHandWidget::PopPlayableCardsForUnit(ALFPTacticsUnit* Unit)
{
	CurrentPlayableUnit = Unit;
	ApplyUnitPlayablePopups();
}

void ULFPCardHandWidget::ResetUnitPlayablePopups()
{
	CurrentPlayableUnit = nullptr;

	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (CardWidget)
		{
			CardWidget->SetPopupReasonActive(ELFPCardPopupReason::UnitPlayable, false);
		}
	}
}

void ULFPCardHandWidget::ResetHoverPopups()
{
	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (CardWidget)
		{
			CardWidget->SetPopupReasonActive(ELFPCardPopupReason::Hover, false);
		}
	}
}

void ULFPCardHandWidget::ResetAllCardPopups()
{
	CurrentPlayableUnit = nullptr;
	ResetDisplayedCardPopups();
}

void ULFPCardHandWidget::ResetDisplayedCardPopups()
{
	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (CardWidget)
		{
			CardWidget->ResetPopupReasons();
		}
	}
}

void ULFPCardHandWidget::OnCardClickedInHand(const FLFPCardInstance& CardInstance)
{
	if (!TacticsPC || !CardInstance.RuntimeSkill)
	{
		return;
	}

	// 通知 PlayerController 处理手牌点击。
	TSubclassOf<UUserWidget> DragVisualClass;
	FVector2D SourcePosition = FVector2D::ZeroVector;
	FVector2D SourceSize = FVector2D::ZeroVector;
	GetCardDragVisualInfo(CardInstance, DragVisualClass, SourcePosition, SourceSize);

	ResetDisplayedCardPopups();
	TacticsPC->ClearCardUsableUnitPreview();
	TacticsPC->OnHandCardClicked(CardInstance, DragVisualClass, SourcePosition, SourceSize);
	if (TacticsPC->IsCardDragActive())
	{
		SetCardMainContentHiddenForDrag(CardInstance.InstanceID, true);
	}

	// 点击后刷新手牌（当前选中单位对应的可用卡高亮会变化）。
}

void ULFPCardHandWidget::OnCardHoveredInHand(const FLFPCardInstance& CardInstance)
{
	if (TacticsPC && TacticsPC->IsCardDragActive())
	{
		return;
	}

	ResetHoverPopups();

	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (!CardWidget)
		{
			continue;
		}

		const FLFPCardInstance& WidgetCard = CardWidget->GetCardInstance();
		const bool bMatchesHoveredCard = WidgetCard.InstanceID == CardInstance.InstanceID;
		CardWidget->SetPopupReasonActive(ELFPCardPopupReason::Hover, bMatchesHoveredCard);
	}

	if (TacticsPC && CardInstance.RuntimeSkill)
	{
		TacticsPC->PreviewCardUsableUnits(CardInstance);
	}
}

void ULFPCardHandWidget::OnCardUnhoveredInHand()
{
	if (TacticsPC && TacticsPC->IsCardDragActive())
	{
		return;
	}

	ResetHoverPopups();

	if (TacticsPC && !TacticsPC->IsCardDragActive())
	{
		TacticsPC->ClearCardUsableUnitPreview();
	}
}

void ULFPCardHandWidget::OnCardDragStartedInHand(const FLFPCardInstance& CardInstance)
{
	if (!TacticsPC)
	{
		return;
	}

	TacticsPC->ClearCardUsableUnitPreview();

	TSubclassOf<UUserWidget> DragVisualClass;
	FVector2D SourcePosition = FVector2D::ZeroVector;
	FVector2D SourceSize = FVector2D::ZeroVector;
	GetCardDragVisualInfo(CardInstance, DragVisualClass, SourcePosition, SourceSize);

	ResetDisplayedCardPopups();

	TacticsPC->BeginCardDrag(CardInstance, DragVisualClass, true, SourcePosition, SourceSize);
	if (TacticsPC->IsCardDragActive())
	{
		SetCardMainContentHiddenForDrag(CardInstance.InstanceID, true);
	}
}

void ULFPCardHandWidget::OnCardDragEndedInHand()
{
	if (TacticsPC)
	{
		TacticsPC->CancelActiveCardDrag();
	}

	RestoreHiddenDraggedCard();
	RefreshHandDisplay();
}

bool ULFPCardHandWidget::GetCardDragVisualInfo(const FLFPCardInstance& CardInstance,
	TSubclassOf<UUserWidget>& OutDragVisualClass, FVector2D& OutSourcePosition, FVector2D& OutSourceSize)
{
	OutDragVisualClass = nullptr;
	OutSourcePosition = FVector2D::ZeroVector;
	OutSourceSize = FVector2D::ZeroVector;

	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (CardWidget && CardWidget->GetCardInstance().InstanceID == CardInstance.InstanceID)
		{
			OutDragVisualClass = CardWidget->DragVisualClass;
			if (!OutDragVisualClass)
			{
				OutDragVisualClass = CardWidget->GetClass();
			}

			const FGeometry CardGeometry = CardWidget->GetCachedGeometry();
			const FGeometry ViewportGeometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
			const FVector2D LocalSize = CardGeometry.GetLocalSize();
			if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
			{
				return true;
			}

			const FVector2D CardTopLeft = ViewportGeometry.AbsoluteToLocal(
				CardGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D CardBottomRight = ViewportGeometry.AbsoluteToLocal(
				CardGeometry.LocalToAbsolute(LocalSize));

			OutSourcePosition = CardTopLeft;
			OutSourceSize = FVector2D(
				FMath::Abs(CardBottomRight.X - CardTopLeft.X),
				FMath::Abs(CardBottomRight.Y - CardTopLeft.Y));
			return true;
		}
	}

	return false;
}

void ULFPCardHandWidget::ApplyUnitPlayablePopups()
{
	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (!CardWidget)
		{
			continue;
		}

		const bool bCanUseCard = TacticsPC && !TacticsPC->IsCardDragActive() &&
			CurrentPlayableUnit != nullptr &&
			CurrentPlayableUnit->CanUseCard(CardWidget->GetCardInstance());
		CardWidget->SetPopupReasonActive(ELFPCardPopupReason::UnitPlayable, bCanUseCard);
	}
}

void ULFPCardHandWidget::SetCardMainContentHiddenForDrag(int32 CardInstanceID, bool bHidden)
{
	if (CardInstanceID == INDEX_NONE)
	{
		return;
	}

	for (ULFPCardItemWidget* CardWidget : HandCardWidgets)
	{
		if (CardWidget && CardWidget->GetCardInstance().InstanceID == CardInstanceID)
		{
			CardWidget->SetMainContentHiddenForDrag(bHidden);
			HiddenDraggedCardInstanceID = bHidden ? CardInstanceID : INDEX_NONE;
			return;
		}
	}

	if (!bHidden && HiddenDraggedCardInstanceID == CardInstanceID)
	{
		HiddenDraggedCardInstanceID = INDEX_NONE;
	}
}

void ULFPCardHandWidget::RestoreHiddenDraggedCard()
{
	if (HiddenDraggedCardInstanceID == INDEX_NONE)
	{
		return;
	}

	SetCardMainContentHiddenForDrag(HiddenDraggedCardInstanceID, false);
	HiddenDraggedCardInstanceID = INDEX_NONE;
}
