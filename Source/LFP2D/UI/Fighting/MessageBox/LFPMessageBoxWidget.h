#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "LFPMessageBoxWidget.generated.h"

UCLASS()
class LFP2D_API ULFPMessageBoxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 在指定屏幕位置显示消息
	void ShowAt(FVector2D ScreenPosition, const FText& Message);

	// 隐藏消息框
	void Hide();

	// 请求延迟显示（悬停 HoverDelay 秒后在鼠标位置弹出），调用 CancelRequest 可取消
	void RequestShow(const FText& Message);

	// 取消待显示的请求并隐藏消息框
	void CancelRequest();

public:
	// 鼠标位置的额外偏移量（Slate 单位，分辨率无关），可在蓝图中调整
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MessageBox")
	FVector2D PositionOffset = FVector2D(-10.0f, -10.0f);

	// 文本超过此宽度后自动换行（Slate 单位），0 表示不换行
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MessageBox")
	float WrapTextAt = 400.0f;

	// 悬停延迟（秒），在此时间内鼠标未移开则弹出消息框，可在蓝图中配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MessageBox")
	float HoverDelay = 1.0f;

private:
	// 延迟计时器到期后执行显示
	void OnHoverDelayFinished();

	FTimerHandle HoverTimerHandle;
	FText PendingMessage;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;
};
