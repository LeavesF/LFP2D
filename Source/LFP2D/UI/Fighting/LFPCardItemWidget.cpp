#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"

#include "LFP2D/Card/LFPBattleCardComponent.h"
#include "LFP2D/Card/LFPCardDragDropOperation.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Components/Border.h"
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

	if (HighlightBorder)
	{
		HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CardButton)
	{
		CardButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ULFPCardItemWidget::InitializeCardItem(const FLFPCardInstance& InCardInstance, ALFPTacticsPlayerController* InPC)
{
	CardInstance = InCardInstance;
	TacticsPC = InPC;

	if (CardButton)
	{
		CardButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (!CardInstance.IsValid())
	{
		return;
	}

	if (CardIcon)
	{
		CardIcon->SetBrushFromTexture(CardInstance.Definition.Icon);
	}

	if (CardNameText)
	{
		CardNameText->SetText(CardInstance.Definition.DisplayName);
	}

	if (CostText)
	{
		CostText->SetText(FText::AsNumber(CardInstance.Definition.ActionPointCost));
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
			FString CategoryLabel;
			switch (CardInstance.Definition.CardCategory)
			{
			case ELFPCardCategory::GeneralAttack:
				CategoryLabel = TEXT("通用");
				break;
			case ELFPCardCategory::RaceSpecific:
				CategoryLabel = TEXT("种族");
				break;
			case ELFPCardCategory::FullyGeneric:
				CategoryLabel = TEXT("公共");
				break;
			case ELFPCardCategory::NoTarget:
				CategoryLabel = TEXT("无目标");
				break;
			}
			SourceUnitText->SetText(FText::FromString(CategoryLabel));
			SourceUnitText->SetVisibility(ESlateVisibility::Visible);
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

FReply ULFPCardItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		MouseDownPosition = InMouseEvent.GetScreenSpacePosition();
		bIsDragDetecting = true;
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return FReply::Unhandled();
}

FReply ULFPCardItemWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragDetecting && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 没拖拽 → 当点击处理。
		const FVector2D CurrentPos = InMouseEvent.GetScreenSpacePosition();
		const float Distance = FVector2D::Distance(MouseDownPosition, CurrentPos);

		if (Distance < DragThreshold && CardInstance.IsValid())
		{
			bIsDragDetecting = false;
			OnCardClicked.Broadcast(CardInstance);
		}
	}

	bIsDragDetecting = false;
	return FReply::Handled();
}

void ULFPCardItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!bIsDragDetecting || !CardInstance.IsValid())
	{
		return;
	}

	bIsDragDetecting = false;

	ULFPCardDragDropOperation* DragOp = NewObject<ULFPCardDragDropOperation>();
	DragOp->DraggedCard = CardInstance;
	DragOp->Pivot = EDragPivot::CenterCenter;

	if (DragVisualClass)
	{
		DragOp->DefaultDragVisual = CreateWidget(this, DragVisualClass);
	}

	OutOperation = DragOp;
	OnCardDragStarted.Broadcast(CardInstance);
}

void ULFPCardItemWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	OnCardDragEnded.Broadcast();
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
