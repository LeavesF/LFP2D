#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"

void ULFPDeploymentWidget::Setup(const TArray<FLFPUnitEntry>& PartyUnits, ULFPUnitRegistryDataAsset* Registry)
{
	CachedPartyUnits = PartyUnits;
	UnitRegistry = Registry;

	// 初始化放置状态（全部未放置）
	PlacedStates.Empty();
	PlacedStates.SetNum(PartyUnits.Num());
	for (int32 i = 0; i < PlacedStates.Num(); i++)
	{
		PlacedStates[i] = false;
	}

	bConfirmEnabled = false;

	// 通知蓝图刷新 UI
	OnSetupComplete();
}

void ULFPDeploymentWidget::MarkUnitPlaced(int32 PartyIndex, bool bPlaced)
{
	if (!PlacedStates.IsValidIndex(PartyIndex)) return;

	PlacedStates[PartyIndex] = bPlaced;
	OnPlacedStateChanged(PartyIndex, bPlaced);

	// 检查是否全部放置完毕
	bool bAllPlaced = true;
	for (bool State : PlacedStates)
	{
		if (!State) { bAllPlaced = false; break; }
	}
	SetConfirmEnabled(bAllPlaced);
}

void ULFPDeploymentWidget::SetConfirmEnabled(bool bEnabled)
{
	bConfirmEnabled = bEnabled;
	OnConfirmEnabledChanged(bEnabled);
}

void ULFPDeploymentWidget::MarkUnitSelecting(int32 PartyIndex)
{
	// 蓝图可读取此状态做视觉反馈
}

void ULFPDeploymentWidget::OnConfirmButtonClicked()
{
	if (bConfirmEnabled)
	{
		OnConfirmPressed.Broadcast();
	}
}

void ULFPDeploymentWidget::OnUnitIconClicked(int32 PartyIndex)
{
	OnUnitSelected.Broadcast(PartyIndex);
}
