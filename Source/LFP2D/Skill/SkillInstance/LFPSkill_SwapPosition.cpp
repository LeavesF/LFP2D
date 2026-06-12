#include "LFP2D/Skill/SkillInstance/LFPSkill_SwapPosition.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_SwapPosition::ULFPSkill_SwapPosition()
{
	SkillName = FText::FromString(TEXT("换位"));
	SkillDescription = FText::FromString(TEXT("与5格内任意一个单位交换位置。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::SingleUnit;
	ReleaseRangeType = ESkillRangeType::Coverage;
	MaxRange = 5;
	EffectRangeType = ESkillRangeType::Origin;
	bRequireLineOfSight = false;
}

bool ULFPSkill_SwapPosition::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile)
	{
		return false;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	if (!TargetUnit || !TargetUnit->IsAlive() || TargetUnit == Owner)
	{
		return false;
	}

	ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
	return OwnerTile && CanReleaseFrom(OwnerTile, TargetTile);
}

void ULFPSkill_SwapPosition::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile))
	{
		return;
	}

	SwapWithTarget(GetUnitOnTile(TargetTile));
}

bool ULFPSkill_SwapPosition::SwapWithTarget(ALFPTacticsUnit* TargetUnit) const
{
	if (!Owner || !Owner->IsAlive() || !TargetUnit || !TargetUnit->IsAlive() || TargetUnit == Owner)
	{
		return false;
	}

	ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
	ALFPHexTile* TargetTile = TargetUnit->GetCurrentTile();
	if (!OwnerTile || !TargetTile)
	{
		return false;
	}

	const FLFPHexCoordinates OwnerCoordinates = OwnerTile->GetCoordinates();
	const FLFPHexCoordinates TargetCoordinates = TargetTile->GetCoordinates();

	Owner->SetCurrentCoordinates(TargetCoordinates, false);
	TargetUnit->SetCurrentCoordinates(OwnerCoordinates, false);

	OwnerTile->SetIsOccupied(true);
	OwnerTile->SetUnitOnTile(TargetUnit);
	TargetTile->SetIsOccupied(true);
	TargetTile->SetUnitOnTile(Owner);

	Owner->LastCommittedCoordinates = Owner->GetCurrentCoordinates();
	TargetUnit->LastCommittedCoordinates = TargetUnit->GetCurrentCoordinates();
	Owner->MovementRangeTiles.Empty();
	TargetUnit->MovementRangeTiles.Empty();

	if (ALFPTurnManager* TurnManager = Owner->GetTurnManager())
	{
		TurnManager->RefreshAllRuntimeUnitStates();
	}

	return true;
}
