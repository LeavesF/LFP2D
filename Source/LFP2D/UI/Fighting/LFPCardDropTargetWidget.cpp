#include "LFP2D/UI/Fighting/LFPCardDropTargetWidget.h"

#include "LFP2D/Card/LFPCardDragDropOperation.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"

FReply ULFPCardDropTargetWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 不处理点击，让事件穿透到游戏世界。
	return FReply::Unhandled();
}

bool ULFPCardDropTargetWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	// 仅接受卡牌拖拽操作。
	return Cast<ULFPCardDragDropOperation>(InOperation) != nullptr;
}

bool ULFPCardDropTargetWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	ULFPCardDragDropOperation* CardDragOp = Cast<ULFPCardDragDropOperation>(InOperation);
	if (!CardDragOp || !CardDragOp->DraggedCard.IsValid())
	{
		return false;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return false;
	}

	ALFPTacticsPlayerController* TacticsPC = Cast<ALFPTacticsPlayerController>(PC);
	if (!TacticsPC)
	{
		return false;
	}

	const FVector2D LocalDropPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D DropViewportPos = LocalDropPos * ViewportScale;
	const bool bDroppedInNoTargetZone = IsDropInsideNoTargetZone(InDragDropEvent);
	const bool bShouldEndCardDrag = TacticsPC->OnCardDroppedOnViewport(
		CardDragOp->DraggedCard, DropViewportPos, bDroppedInNoTargetZone);
	if (bShouldEndCardDrag)
	{
		TacticsPC->EndCardDrag();
	}
	return true;
}

bool ULFPCardDropTargetWidget::IsDropInsideNoTargetZone(const FDragDropEvent& InDragDropEvent) const
{
	return IsSlatePositionInsideNoTargetZone(InDragDropEvent.GetScreenSpacePosition());
}

bool ULFPCardDropTargetWidget::IsViewportPositionInsideNoTargetZone(FVector2D ViewportPosition) const
{
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D SlatePosition = ViewportScale > KINDA_SMALL_NUMBER
		? ViewportPosition / ViewportScale
		: ViewportPosition;

	return IsSlatePositionInsideNoTargetZone(SlatePosition);
}

bool ULFPCardDropTargetWidget::IsSlatePositionInsideNoTargetZone(FVector2D SlatePosition) const
{
	if (!NoTargetDropZone)
	{
		return false;
	}

	const FGeometry ZoneGeometry = NoTargetDropZone->GetCachedGeometry();
	const FVector2D ZoneSize = ZoneGeometry.GetLocalSize();
	if (ZoneSize.X <= 0.0f || ZoneSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D ZoneLocalDropPos = ZoneGeometry.AbsoluteToLocal(SlatePosition);
	return ZoneLocalDropPos.X >= 0.0f
		&& ZoneLocalDropPos.Y >= 0.0f
		&& ZoneLocalDropPos.X <= ZoneSize.X
		&& ZoneLocalDropPos.Y <= ZoneSize.Y;
}
