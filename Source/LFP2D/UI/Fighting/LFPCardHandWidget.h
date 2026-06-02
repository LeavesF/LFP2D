#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPCardHandWidget.generated.h"

class UHorizontalBox;
class UWrapBox;
class UTextBlock;
class UButton;
class ULFPCardItemWidget;
class ULFPBattleCardComponent;
class ALFPTacticsPlayerController;
class ALFPTacticsUnit;

UCLASS()
class LFP2D_API ULFPCardHandWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULFPCardHandWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Card Hand")
	void InitializeFromBattleCardComponent(ULFPBattleCardComponent* InCardComponent, ALFPTacticsPlayerController* InPC);

	UFUNCTION(BlueprintCallable, Category = "Card Hand")
	void RefreshHandDisplay();

	UFUNCTION(BlueprintCallable, Category = "Card Hand")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Card Hand")
	void Hide();

	UFUNCTION(BlueprintCallable, Category = "Card Hand|Popup")
	void PopPlayableCardsForUnit(ALFPTacticsUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Card Hand|Popup")
	void ResetUnitPlayablePopups();

	UFUNCTION(BlueprintCallable, Category = "Card Hand|Popup")
	void ResetHoverPopups();

	UFUNCTION(BlueprintCallable, Category = "Card Hand|Popup")
	void ResetAllCardPopups();

	UFUNCTION(BlueprintPure, Category = "Card Hand")
	int32 GetHandCardCount() const { return HandCardWidgets.Num(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnCardClickedInHand(const FLFPCardInstance& CardInstance);

	UFUNCTION()
	void OnCardHoveredInHand(const FLFPCardInstance& CardInstance);

	UFUNCTION()
	void OnCardUnhoveredInHand();

	UFUNCTION()
	void OnCardDragStartedInHand(const FLFPCardInstance& CardInstance);

	UFUNCTION()
	void OnCardDragEndedInHand();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Hand")
	TSubclassOf<ULFPCardItemWidget> CardItemWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> CardContainer;

	// 使用 WrapBox 时指定每行最大卡片数；HorizontalBox 时忽略。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Hand")
	int32 MaxCardsPerRow = 8;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HandCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DrawPileCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiscardPileCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ExhaustPileCountText;

private:
	void ApplyUnitPlayablePopups();
	void ResetDisplayedCardPopups();
	bool GetCardDragVisualInfo(const FLFPCardInstance& CardInstance, TSubclassOf<UUserWidget>& OutDragVisualClass,
		FVector2D& OutSourcePosition, FVector2D& OutSourceSize);
	void SetCardMainContentHiddenForDrag(int32 CardInstanceID, bool bHidden);
	void RestoreHiddenDraggedCard();

	UPROPERTY()
	TObjectPtr<ULFPBattleCardComponent> CardComponent;

	UPROPERTY()
	TObjectPtr<ALFPTacticsPlayerController> TacticsPC;

	UPROPERTY()
	TObjectPtr<ALFPTacticsUnit> CurrentPlayableUnit;

	UPROPERTY()
	TArray<TObjectPtr<ULFPCardItemWidget>> HandCardWidgets;

	int32 HiddenDraggedCardInstanceID = INDEX_NONE;
};
