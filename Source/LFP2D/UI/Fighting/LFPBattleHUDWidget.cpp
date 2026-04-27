#include "LFP2D/UI/Fighting/LFPBattleHUDWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "Components/CanvasPanel.h"

void ULFPBattleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// TurnSpeedList 默认可见，其余子面板初始隐藏
	if (SkillSelectionWidget)
	{
		SkillSelectionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DeploymentWidget)
	{
		DeploymentWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (BattleResultWidget)
	{
		BattleResultWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPBattleHUDWidget::ShowTurnSpeedList()
{
	if (TurnSpeedListWidget)
	{
		TurnSpeedListWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULFPBattleHUDWidget::HideTurnSpeedList()
{
	if (TurnSpeedListWidget)
	{
		TurnSpeedListWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPBattleHUDWidget::ShowSkillSelection()
{
	if (SkillSelectionWidget)
	{
		SkillSelectionWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULFPBattleHUDWidget::HideSkillSelection()
{
	if (SkillSelectionWidget)
	{
		SkillSelectionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPBattleHUDWidget::ShowDeploymentWidget()
{
	if (DeploymentWidget)
	{
		DeploymentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULFPBattleHUDWidget::HideDeploymentWidget()
{
	if (DeploymentWidget)
	{
		DeploymentWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPBattleHUDWidget::ShowBattleResultWidget()
{
	if (BattleResultWidget)
	{
		BattleResultWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULFPBattleHUDWidget::HideBattleResultWidget()
{
	if (BattleResultWidget)
	{
		BattleResultWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
