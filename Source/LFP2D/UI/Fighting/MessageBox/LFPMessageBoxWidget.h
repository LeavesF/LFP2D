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

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;
};
