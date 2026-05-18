#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPDeploymentDeckPreviewWidget.generated.h"

class ALFPTacticsPlayerController;
class ULFPCardItemWidget;
class UTextBlock;
class UWrapBox;

UCLASS()
class LFP2D_API ULFPDeploymentDeckPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Deployment|Deck")
	void RefreshDeckPreview(const TArray<FLFPCardInstance>& Cards, ALFPTacticsPlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Deployment|Deck")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Deployment|Deck")
	void Hide();

	UFUNCTION(BlueprintCallable, Category = "Deployment|Deck")
	void ToggleVisibility();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> CardContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardCountText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment|Deck")
	TSubclassOf<ULFPCardItemWidget> CardItemWidgetClass;
};
