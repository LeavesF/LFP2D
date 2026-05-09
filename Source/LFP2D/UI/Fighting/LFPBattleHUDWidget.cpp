#include "LFP2D/UI/Fighting/LFPBattleHUDWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "LFP2D/UI/Fighting/LFPCurrentUnitInfoWidget.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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
	if (CurrentUnitInfoWidget)
	{
		CurrentUnitInfoWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateEnergyBar();
}

void ULFPBattleHUDWidget::NativeDestruct()
{
	SetTurnManager(nullptr);

	Super::NativeDestruct();
}

void ULFPBattleHUDWidget::InitializeEnergyBar(ALFPTurnManager* TurnManager)
{
	SetTurnManager(TurnManager);
	UpdateEnergyBar();
}

void ULFPBattleHUDWidget::InitializeTurnInfo(ALFPTurnManager* TurnManager)
{
	SetTurnManager(TurnManager);
	UpdateTurnInfo();
}

void ULFPBattleHUDWidget::UpdateEnergyBar()
{
	if (!EnergyProgressBar)
	{
		return;
	}

	const int32 CurrentAP = TurnManagerRef
		? TurnManagerRef->GetFactionAP(EUnitAffiliation::UA_Player)
		: 0;
	const int32 ClampedAP = FMath::Clamp(CurrentAP, 0, 3);
	EnergyProgressBar->SetPercent(static_cast<float>(ClampedAP) / 3.0f);
}

void ULFPBattleHUDWidget::OnFactionAPChanged(EUnitAffiliation Faction, int32 NewAP)
{
	if (Faction != EUnitAffiliation::UA_Player || !EnergyProgressBar)
	{
		return;
	}

	const int32 ClampedAP = FMath::Clamp(NewAP, 0, 3);
	EnergyProgressBar->SetPercent(static_cast<float>(ClampedAP) / 3.0f);
}

void ULFPBattleHUDWidget::OnTurnChanged()
{
	UpdateTurnInfo();
}

void ULFPBattleHUDWidget::OnPhaseChanged(EBattlePhase NewPhase)
{
	UpdateRoundText();
	UpdatePhaseText(NewPhase);
}

void ULFPBattleHUDWidget::SetTurnManager(ALFPTurnManager* TurnManager)
{
	if (TurnManagerRef == TurnManager)
	{
		return;
	}

	if (TurnManagerRef)
	{
		TurnManagerRef->OnFactionAPChanged.RemoveDynamic(this, &ULFPBattleHUDWidget::OnFactionAPChanged);
		TurnManagerRef->OnTurnChanged.RemoveDynamic(this, &ULFPBattleHUDWidget::OnTurnChanged);
		TurnManagerRef->OnPhaseChanged.RemoveDynamic(this, &ULFPBattleHUDWidget::OnPhaseChanged);
	}

	TurnManagerRef = TurnManager;

	if (TurnManagerRef)
	{
		TurnManagerRef->OnFactionAPChanged.AddUniqueDynamic(this, &ULFPBattleHUDWidget::OnFactionAPChanged);
		TurnManagerRef->OnTurnChanged.AddUniqueDynamic(this, &ULFPBattleHUDWidget::OnTurnChanged);
		TurnManagerRef->OnPhaseChanged.AddUniqueDynamic(this, &ULFPBattleHUDWidget::OnPhaseChanged);
	}
}

void ULFPBattleHUDWidget::UpdateTurnInfo()
{
	UpdateRoundText();

	if (TurnManagerRef)
	{
		UpdatePhaseText(TurnManagerRef->GetCurrentPhase());
	}
	else if (PhaseText)
	{
		PhaseText->SetText(FText::GetEmpty());
	}
}

void ULFPBattleHUDWidget::UpdateRoundText()
{
	if (!RoundText)
	{
		return;
	}

	if (!TurnManagerRef ||
		TurnManagerRef->GetCurrentPhase() == EBattlePhase::BP_Deployment ||
		TurnManagerRef->GetCurrentRound() <= 0)
	{
		RoundText->SetText(FText::GetEmpty());
		return;
	}

	RoundText->SetText(FText::FromString(FString::Printf(TEXT("%d"), TurnManagerRef->GetCurrentRound())));
}

void ULFPBattleHUDWidget::UpdatePhaseText(EBattlePhase NewPhase)
{
	if (!PhaseText)
	{
		return;
	}

	if (const UEnum* PhaseEnum = StaticEnum<EBattlePhase>())
	{
		PhaseText->SetText(PhaseEnum->GetDisplayNameTextByValue(static_cast<int64>(NewPhase)));
	}
	else
	{
		PhaseText->SetText(FText::GetEmpty());
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

void ULFPBattleHUDWidget::ClearSelectedSkill()
{
	if (SkillSelectionWidget)
	{
		SkillSelectionWidget->ClearSelectedSkill();
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

void ULFPBattleHUDWidget::InitializeCurrentUnitInfo(ALFPTurnManager* TurnManager)
{
	if (CurrentUnitInfoWidget)
	{
		CurrentUnitInfoWidget->InitializeCurrentUnitInfo(TurnManager);
	}
}

void ULFPBattleHUDWidget::ShowCurrentUnitInfo()
{
	if (CurrentUnitInfoWidget)
	{
		CurrentUnitInfoWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULFPBattleHUDWidget::HideCurrentUnitInfo()
{
	if (CurrentUnitInfoWidget)
	{
		CurrentUnitInfoWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULFPBattleHUDWidget::SetCurrentUnitInfoUnit(ALFPTacticsUnit* Unit)
{
	if (CurrentUnitInfoWidget)
	{
		CurrentUnitInfoWidget->BindToUnit(Unit);
	}
}
