#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPCardItemWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class ULFPSkillBase;
class ALFPTacticsPlayerController;
class ULFPCardDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCardClickedSignature, const FLFPCardInstance&, CardInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCardHoveredSignature, const FLFPCardInstance&, CardInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardUnhoveredSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCardDragStartedSignature, const FLFPCardInstance&, CardInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardDragEndedSignature);

UCLASS()
class LFP2D_API ULFPCardItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULFPCardItemWidget(const FObjectInitializer& ObjectInitializer);

	void InitializeCardItem(const FLFPCardInstance& InCardInstance, ALFPTacticsPlayerController* InPC);

	UFUNCTION(BlueprintPure, Category = "Card")
	const FLFPCardInstance& GetCardInstance() const { return CardInstance; }

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetHighlighted(bool bHighlighted);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardClickedSignature OnCardClicked;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardHoveredSignature OnCardHovered;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardUnhoveredSignature OnCardUnhovered;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardDragStartedSignature OnCardDragStarted;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardDragEndedSignature OnCardDragEnded;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CardNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SourceUnitText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FLinearColor NormalTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FLinearColor HighlightedTint = FLinearColor(1.0f, 0.9f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	TSubclassOf<UUserWidget> DragVisualClass;

	// 鼠标按下后移动超过此距离才触发拖拽（像素）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	float DragThreshold = 8.0f;

private:
	FLFPCardInstance CardInstance;

	UPROPERTY()
	TObjectPtr<ALFPTacticsPlayerController> TacticsPC;

	FVector2D MouseDownPosition;
	bool bIsDragDetecting = false;
};
