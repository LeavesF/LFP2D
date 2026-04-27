#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPBattleHUDWidget.generated.h"

class UCanvasPanel;
class ULFPTurnSpeedListWidget;
class ULFPSkillSelectionWidget;
class ULFPDeploymentWidget;
class ULFPBattleResultWidget;

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

	// === TurnSpeedList ===
	void ShowTurnSpeedList();
	void HideTurnSpeedList();
	ULFPTurnSpeedListWidget* GetTurnSpeedListWidget() const { return TurnSpeedListWidget; }

	// === SkillSelection ===
	void ShowSkillSelection();
	void HideSkillSelection();
	ULFPSkillSelectionWidget* GetSkillSelectionWidget() const { return SkillSelectionWidget; }

	// === Deployment ===
	void ShowDeploymentWidget();
	void HideDeploymentWidget();
	ULFPDeploymentWidget* GetDeploymentWidget() const { return DeploymentWidget; }

	// === BattleResult ===
	void ShowBattleResultWidget();
	void HideBattleResultWidget();
	ULFPBattleResultWidget* GetBattleResultWidget() const { return BattleResultWidget; }

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPTurnSpeedListWidget> TurnSpeedListWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPSkillSelectionWidget> SkillSelectionWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPDeploymentWidget> DeploymentWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULFPBattleResultWidget> BattleResultWidget;
};
