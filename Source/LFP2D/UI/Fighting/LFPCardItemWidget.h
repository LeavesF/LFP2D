#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPCardItemWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class ULFPSkillBase;
class ALFPTacticsPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCardClickedSignature, const FLFPCardInstance&, CardInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCardHoveredSignature, const FLFPCardInstance&, CardInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardUnhoveredSignature);

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
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void OnCardButtonClicked();

public:
	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardClickedSignature OnCardClicked;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardHoveredSignature OnCardHovered;

	UPROPERTY(BlueprintAssignable, Category = "Card")
	FCardUnhoveredSignature OnCardUnhovered;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CardNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	// 单位来源指示（可选，单位专属卡显示来源单位名）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SourceUnitText;

	// 高亮边框（可选）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FLinearColor NormalTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FLinearColor HighlightedTint = FLinearColor(1.0f, 0.9f, 0.3f);

private:
	FLFPCardInstance CardInstance;

	UPROPERTY()
	TObjectPtr<ALFPTacticsPlayerController> TacticsPC;
};
