#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPDeploymentWidget.generated.h"

class ULFPUnitRegistryDataAsset;
class ULFPUnitSlotWidget;
class ULFPDeploymentDeckPreviewWidget;
class UWrapBox;
class UButton;
class ALFPTacticsPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeploymentUnitClicked, int32, UnitIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentConfirmed);

UCLASS()
class LFP2D_API ULFPDeploymentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void Setup(const TArray<FLFPUnitEntry>& PartyUnits, const TArray<FLFPUnitEntry>& ReserveUnits, ULFPUnitRegistryDataAsset* Registry);

	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void SetUnitDeployed(int32 UnitIndex, bool bIsDeployed);

	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void ClearAllSelections();

	UFUNCTION(BlueprintCallable, Category = "Deployment|Deck")
	void RefreshDeckPreview(const TArray<FLFPCardInstance>& Cards, ALFPTacticsPlayerController* PC);

	UPROPERTY(BlueprintAssignable)
	FOnDeploymentUnitClicked OnUnitClicked;

	UPROPERTY(BlueprintAssignable)
	FOnDeploymentConfirmed OnConfirmPressed;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_Units;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_Party;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_Reserve;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ToggleDeckPreview;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<ULFPDeploymentDeckPreviewWidget> DeckPreviewWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment")
	TSubclassOf<ULFPUnitSlotWidget> UnitSlotWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<FLFPUnitEntry> CachedUnits;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TArray<bool> CachedDeploymentStates;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;

	int32 SelectedUnitIndex = -1;

	UPROPERTY()
	TArray<TObjectPtr<ULFPUnitSlotWidget>> UnitSlotWidgets;

private:
	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnUnitSlotClicked(ULFPUnitSlotWidget* ClickedSlot);

	UFUNCTION()
	void OnToggleDeckPreviewClicked();

	UWrapBox* GetUnitsContainer() const;
	void CreateUnitSlots(UWrapBox* Container);
	void ClearContainer(UWrapBox* Container, TArray<TObjectPtr<ULFPUnitSlotWidget>>& OutWidgets);
};
