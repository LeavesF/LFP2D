#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPCurrentUnitInfoWidget.generated.h"

class ALFPTacticsUnit;
class ALFPTurnManager;
class ULFPBuffIconWidget;
class ULFPCardDataAsset;
class ULFPCardItemWidget;
class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UTexture2D;

UCLASS()
class LFP2D_API ULFPCurrentUnitInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void InitializeCurrentUnitInfo(ALFPTurnManager* InTurnManager = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void SetTurnManager(ALFPTurnManager* InTurnManager);

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void RefreshFromTurnManager();

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void BindToUnit(ALFPTacticsUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void UnbindFromUnit();

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void RefreshUnitInfo();

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void RefreshHealth();

	UFUNCTION(BlueprintCallable, Category = "Current Unit")
	void RefreshStats();

	UFUNCTION(BlueprintCallable, Category = "Current Unit|Buff")
	void RefreshBuffIcons();

	UFUNCTION(BlueprintCallable, Category = "Current Unit|Cards")
	void RefreshCarriedCards();

	UFUNCTION(BlueprintPure, Category = "Current Unit")
	ALFPTacticsUnit* GetBoundUnit() const { return BoundUnit; }

	UFUNCTION(BlueprintPure, Category = "Current Unit")
	ALFPTurnManager* GetTurnManager() const { return TurnManagerRef; }

	// ==== 检查模式 ====
	UFUNCTION(BlueprintCallable, Category = "Current Unit|Inspection")
	void SetInspectionMode(bool bInspection);

	UFUNCTION(BlueprintPure, Category = "Current Unit|Inspection")
	bool IsInInspectionMode() const { return bIsInspectionMode; }

protected:
	UFUNCTION()
	void OnTurnChanged();

	UFUNCTION()
	void OnPhaseChanged(EBattlePhase NewPhase);

	UFUNCTION()
	void OnHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION()
	void OnUnitDeath();

	UFUNCTION()
	void OnBuffListChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Current Unit")
	void OnBoundUnitChanged(ALFPTacticsUnit* NewUnit);

	UFUNCTION(BlueprintImplementableEvent, Category = "Current Unit")
	void OnUnitInfoRefreshed(ALFPTacticsUnit* Unit);

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit", meta = (BindWidgetOptional))
	TObjectPtr<UImage> UnitIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnitNameText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TierText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AffiliationText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Health", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Health", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackTypeText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MoveText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeedText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackCountText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionCountText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhysicalBlockText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpellDefenseText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Stats", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Buff", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BuffContainer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit|Buff")
	TSubclassOf<ULFPBuffIconWidget> BuffIconWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit|Buff", meta = (ClampMin = "0"))
	int32 MaxBuffIcons = 8;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Cards", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CarriedCardContainer;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Cards", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarriedCardCountText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit|Cards")
	TSubclassOf<ULFPCardItemWidget> CarriedCardItemWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit|Cards", meta = (ClampMin = "0"))
	int32 MaxCarriedCards = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit")
	bool bHideOutsideActionPhase = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Current Unit")
	bool bHideDuringEnemyTurn = true;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Inspection")
	bool bIsInspectionMode = false;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit|Inspection", meta = (BindWidgetOptional))
	TObjectPtr<UImage> InspectionOverlay;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit")
	TObjectPtr<ALFPTacticsUnit> BoundUnit;

	UPROPERTY(BlueprintReadOnly, Category = "Current Unit")
	TObjectPtr<ALFPTurnManager> TurnManagerRef;

	UPROPERTY()
	TArray<FLFPCardInstance> DisplayedCarriedCards;

private:
	void FindTurnManager();
	void ClearUnitInfo();
	void UnbindFromTurnManager();
	void SetOptionalText(UTextBlock* TextBlock, const FText& Text) const;
	FText GetUnitDisplayName(ALFPTacticsUnit* Unit) const;
	UTexture2D* GetUnitDisplayIcon(ALFPTacticsUnit* Unit) const;
	void BuildCarriedCardInstances(TArray<FLFPCardInstance>& OutCards);
	bool AddCarriedCardData(const TSoftObjectPtr<ULFPCardDataAsset>& CardData, TArray<FLFPCardInstance>& OutCards);
	bool AddCarriedCardDefinition(const FLFPCardDefinition& Definition, TArray<FLFPCardInstance>& OutCards);
	bool AddCarriedCardSkillClass(TSubclassOf<ULFPSkillBase> SkillClass, TArray<FLFPCardInstance>& OutCards);

	FTimerHandle RefreshTimerHandle;
};
