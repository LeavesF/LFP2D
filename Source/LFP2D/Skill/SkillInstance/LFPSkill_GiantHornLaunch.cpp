#include "LFP2D/Skill/SkillInstance/LFPSkill_GiantHornLaunch.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_GiantHornLaunch::ULFPSkill_GiantHornLaunch()
{
	SkillName = FText::FromString(TEXT("巨角挑飞"));
	SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，若自己身后一格可以进入，则将目标挑到自己身后一格，并造成一次 100% 攻击力的物理伤害。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::SingleEnemy;
	ReleaseRangeType = ESkillRangeType::Coverage;
	MaxRange = 1;
	EffectRangeType = ESkillRangeType::Origin;
	bTriggersEnemyMissBuffOnMiss = true;
}

bool ULFPSkill_GiantHornLaunch::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile)
	{
		return false;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	if (!TargetUnit || !TargetUnit->IsAlive() || !IsHostileTarget(TargetUnit))
	{
		return false;
	}

	ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
	if (!OwnerTile || !CanReleaseFrom(OwnerTile, TargetTile))
	{
		return false;
	}

	return true;
}

void ULFPSkill_GiantHornLaunch::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile) || !Owner)
	{
		return;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	if (!TargetUnit)
	{
		return;
	}

	ALFPHexTile* DestinationTile = nullptr;
	if (GetBehindDestination(TargetTile, DestinationTile))
	{
		TargetUnit->SetCurrentCoordinates(DestinationTile->GetCoordinates(), true);
		TargetUnit->LastCommittedCoordinates = TargetUnit->GetCurrentCoordinates();
		TargetUnit->MovementRangeTiles.Empty();
	}

	DealOwnerSkillDamage(TargetUnit);
}

float ULFPSkill_GiantHornLaunch::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return DamageScale;
}

ELFPAttackType ULFPSkill_GiantHornLaunch::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return ELFPAttackType::AT_Physical;
}

bool ULFPSkill_GiantHornLaunch::GetBehindDestination(ALFPHexTile* TargetTile, ALFPHexTile*& OutDestinationTile) const
{
	OutDestinationTile = nullptr;

	if (!Owner || !TargetTile)
	{
		return false;
	}

	ALFPHexGridManager* GridManager = Owner->GetGridManager();
	if (!GridManager)
	{
		return false;
	}

	const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
	const FLFPHexCoordinates TargetCoord = TargetTile->GetCoordinates();
	if (FLFPHexCoordinates::Distance(OwnerCoord, TargetCoord) != 1)
	{
		return false;
	}

	const FLFPHexCoordinates DirectionToTarget(TargetCoord.Q - OwnerCoord.Q, TargetCoord.R - OwnerCoord.R);
	bool bValidDirection = false;
	for (const FLFPHexCoordinates& Direction : ALFPHexGridManager::HexDirections)
	{
		if (DirectionToTarget == Direction)
		{
			bValidDirection = true;
			break;
		}
	}

	if (!bValidDirection)
	{
		return false;
	}

	const FLFPHexCoordinates DestinationCoord(OwnerCoord.Q - DirectionToTarget.Q, OwnerCoord.R - DirectionToTarget.R);
	ALFPHexTile* DestinationTile = GridManager->GetTileAtCoordinates(DestinationCoord);
	if (!DestinationTile || !DestinationTile->IsWalkable() || DestinationTile->IsOccupied() || DestinationTile->GetUnitOnTile())
	{
		return false;
	}

	OutDestinationTile = DestinationTile;
	return true;
}
