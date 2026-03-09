#include "LFP2D/UI/WorldMap/LFPUnitReplacementWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Kismet/GameplayStatics.h"

void ULFPUnitReplacementWidget::Setup(
	const FLFPUnitEntry& NewUnit,
	const TArray<FLFPUnitEntry>& CurrentParty,
	const TArray<FLFPUnitEntry>& CurrentReserve,
	ULFPUnitRegistryDataAsset* Registry)
{
	PendingUnit = NewUnit;
	UnitRegistry = Registry;
	// 蓝图子类中可直接读取 GameInstance 的 PartyUnits/ReserveUnits 来显示 UI
}

void ULFPUnitReplacementWidget::ReplacePartySlot(int32 SlotIndex)
{
	if (ULFPGameInstance* GI = Cast<ULFPGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->ReplacePartyUnit(SlotIndex, PendingUnit);
	}
	OnReplacementComplete.Broadcast();
}

void ULFPUnitReplacementWidget::ReplaceReserveSlot(int32 SlotIndex)
{
	if (ULFPGameInstance* GI = Cast<ULFPGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->ReplaceReserveUnit(SlotIndex, PendingUnit);
	}
	OnReplacementComplete.Broadcast();
}

void ULFPUnitReplacementWidget::DiscardNewUnit()
{
	// 直接放弃，不修改队伍
	OnReplacementComplete.Broadcast();
}
