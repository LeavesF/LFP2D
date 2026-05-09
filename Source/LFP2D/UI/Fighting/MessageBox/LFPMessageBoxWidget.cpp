#include "LFP2D/UI/Fighting/MessageBox/LFPMessageBoxWidget.h"

void ULFPMessageBoxWidget::ShowAt(FVector2D ScreenPosition, const FText& Message)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	// 确保已添加到视口
	if (!IsInViewport())
	{
		AddToViewport(100);
	}

	// 强制布局更新以获取最新尺寸
	ForceLayoutPrepass();

	// 右下角对齐鼠标位置
	const FVector2D WidgetSize = GetDesiredSize();
	const FVector2D AdjustedPosition = ScreenPosition - WidgetSize;

	SetPositionInViewport(AdjustedPosition, false);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void ULFPMessageBoxWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
