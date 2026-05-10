#include "LFP2D/UI/Fighting/MessageBox/LFPMessageBoxWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void ULFPMessageBoxWidget::ShowAt(FVector2D ScreenPosition, const FText& Message)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
		MessageText->SetAutoWrapText(WrapTextAt > 0.0f);
		if (WrapTextAt > 0.0f)
		{
			MessageText->SetWrapTextAt(WrapTextAt);
		}
	}

	// 确保已添加到视口
	if (!IsInViewport())
	{
		AddToViewport(100);
	}

	// 强制布局更新以获取最新尺寸
	ForceLayoutPrepass();

	// 右下角对齐鼠标位置
	// 统一在 Slate 空间计算，确保 offset 在不同分辨率下视觉一致
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
	const FVector2D SlatePosition = ScreenPosition / ViewportScale;
	const FVector2D WidgetSize = GetDesiredSize();
	const FVector2D AdjustedPosition = SlatePosition - WidgetSize + PositionOffset;

	SetPositionInViewport(AdjustedPosition, false);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void ULFPMessageBoxWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPMessageBoxWidget::RequestShow(const FText& Message)
{
	PendingMessage = Message;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoverTimerHandle);
		World->GetTimerManager().SetTimer(
			HoverTimerHandle,
			this,
			&ULFPMessageBoxWidget::OnHoverDelayFinished,
			HoverDelay,
			false);
	}
}

void ULFPMessageBoxWidget::CancelRequest()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoverTimerHandle);
	}
	Hide();
}

void ULFPMessageBoxWidget::OnHoverDelayFinished()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		float MouseX, MouseY;
		PC->GetMousePosition(MouseX, MouseY);
		ShowAt(FVector2D(MouseX, MouseY), PendingMessage);
	}
}
