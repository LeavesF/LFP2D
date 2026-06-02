#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "LFPCardDragLineWidget.generated.h"

class UPaperSprite;

UCLASS()
class LFP2D_API ULFPCardDragLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULFPCardDragLineWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Card|Drag")
	void SetLineSprite(UPaperSprite* InSprite);

	UFUNCTION(BlueprintCallable, Category = "Card|Drag")
	void ConfigureLine(float InLineThickness, FLinearColor InLineTint, bool bInTileSpriteAlongLine);

	UFUNCTION(BlueprintCallable, Category = "Card|Drag")
	void SetLine(FVector2D InStart, FVector2D InEnd);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Drag")
	FLinearColor LineTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Drag", meta = (ClampMin = "1.0"))
	float LineThickness = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Drag")
	bool bTileSpriteAlongLine = true;

private:
	UPROPERTY()
	TObjectPtr<UPaperSprite> LineSprite;

	FSlateBrush LineBrush;
	FVector2D Start = FVector2D::ZeroVector;
	FVector2D End = FVector2D::ZeroVector;
};
