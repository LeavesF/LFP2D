#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPBattleHUDWidget.generated.h"

class UCanvasPanel;
class ULFPTurnSpeedListWidget;
class ULFPSkillSelectionWidget;
class ULFPDeploymentWidget;
class ULFPBattleResultWidget;
class ULFPCurrentUnitInfoWidget;
class ULFPCardHandWidget;
class ULFPPendingCardWidget;
class ULFPCardDropTargetWidget;
class UButton;
class UImage;
class UProgressBar;
class UTextBlock;

struct FLFPCardInstance;
class ALFPTacticsUnit;
class ALFPTacticsPlayerController;
class ALFPTurnManager;

/**
 * 战斗 HUD Widget：统一管理所有战斗 UI 子面板的显示和隐藏
 * 子 Widget 在蓝图中作为 BindWidget 预先放置，由 CanvasPanel 控制 Z 序
 */
UCLASS()
class LFP2D_API ULFPBattleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitializeEnergyBar(ALFPTurnManager* TurnManager = nullptr);
	void InitializeTurnInfo(ALFPTurnManager* TurnManager = nullptr);
	void UpdateEnergyBar();

	// === TurnSpeedList ===
	void ShowTurnSpeedList();
	void HideTurnSpeedList();
	ULFPTurnSpeedListWidget* GetTurnSpeedListWidget() const { return TurnSpeedListWidget; }

	// === SkillSelection ===
	void ShowSkillSelection();
	void HideSkillSelection();
	void ClearSelectedSkill();
	ULFPSkillSelectionWidget* GetSkillSelectionWidget() const { return SkillSelectionWidget; }

	// === Deployment ===
	void ShowDeploymentWidget();
	void HideDeploymentWidget();
	ULFPDeploymentWidget* GetDeploymentWidget() const { return DeploymentWidget; }

	// === BattleResult ===
	void ShowBattleResultWidget();
	void HideBattleResultWidget();
	ULFPBattleResultWidget* GetBattleResultWidget() const { return BattleResultWidget; }

	// === CurrentUnitInfo ===
	void InitializeCurrentUnitInfo(ALFPTurnManager* TurnManager = nullptr);
	void ShowCurrentUnitInfo();
	void HideCurrentUnitInfo();
	void SetCurrentUnitInfoUnit(ALFPTacticsUnit* Unit);
	ULFPCurrentUnitInfoWidget* GetCurrentUnitInfoWidget() const { return CurrentUnitInfoWidget; }

	// === CardHand ===
	void ShowCardHand();
	void HideCardHand();
	void RefreshCardHand();
	ULFPCardHandWidget* GetCardHandWidget() const { return CardHandWidget; }

	// === PendingCard ===
	void ShowPendingCard(const FLFPCardInstance& Card);
	void HidePendingCard();
	ULFPPendingCardWidget* GetPendingCardWidget() const { return PendingCardWidget; }
	void SetCardDropTargetActive(bool bActive);

	// ==== 检查模式 ====
	void EnterInspectionMode(ALFPTacticsUnit* InspectedUnit, ALFPTacticsPlayerController* PC);
	void ExitInspectionMode(ALFPTacticsPlayerController* PC);

	// ==== End Turn Button ====
	UFUNCTION()
	void OnEndTurnClicked();
	void SetEndTurnButtonVisible(bool bVisible);

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnFactionAPChanged(EUnitAffiliation Faction, int32 NewAP);
	UFUNCTION()
	void OnTurnChanged();
	UFUNCTION()
	void OnPhaseChanged(EBattlePhase NewPhase);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPTurnSpeedListWidget> TurnSpeedListWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoundText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPSkillSelectionWidget> SkillSelectionWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPDeploymentWidget> DeploymentWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPBattleResultWidget> BattleResultWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULFPCurrentUnitInfoWidget> CurrentUnitInfoWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULFPCardHandWidget> CardHandWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULFPPendingCardWidget> PendingCardWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULFPCardDropTargetWidget> CardDropTargetWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> EnergyBarBackground;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnergyProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<ALFPTurnManager> TurnManagerRef;

private:
	void SetTurnManager(ALFPTurnManager* TurnManager);
	void UpdateTurnInfo();
	void UpdateRoundText();
	void UpdatePhaseText(EBattlePhase NewPhase);
};
