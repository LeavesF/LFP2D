#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPPendingCardWidget.generated.h"

class UImage;
class UTextBlock;
class ULFPSkillBase;

UCLASS()
class LFP2D_API ULFPPendingCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Pending Card")
	void ShowPendingCard(const FLFPCardInstance& InCard);

	UFUNCTION(BlueprintCallable, Category = "Pending Card")
	void HidePendingCard();

	UFUNCTION(BlueprintPure, Category = "Pending Card")
	bool HasPendingCard() const { return bHasPendingCard; }

	UFUNCTION(BlueprintPure, Category = "Pending Card")
	const FLFPCardInstance& GetPendingCard() const { return PendingCard; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

private:
	FLFPCardInstance PendingCard;
	bool bHasPendingCard = false;
};
