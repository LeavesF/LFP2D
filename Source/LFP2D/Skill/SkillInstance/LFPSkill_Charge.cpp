#include "LFP2D/Skill/SkillInstance/LFPSkill_Charge.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_Charge::ULFPSkill_Charge()
{
	SkillName = FText::FromString(TEXT("冲撞"));
	SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，使其沿远离释放者的方向位移 1 格，并造成一次 100% 攻击力的物理伤害。若前方格不可进入，则仅造成伤害。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::SingleEnemy;
	ReleaseRangeType = ESkillRangeType::Coverage;
	MaxRange = 1;
	EffectRangeType = ESkillRangeType::Origin;
	bTriggersEnemyMissBuffOnMiss = true;
}

bool ULFPSkill_Charge::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_Charge::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile) || !Owner)
	{
		return;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	ALFPHexTile* DestinationTile = nullptr;
	if (!TargetUnit)
	{
		return;
	}

	if (GetPushDestination(TargetTile, DestinationTile))
	{
		TargetUnit->SetCurrentCoordinates(DestinationTile->GetCoordinates(), true);
		TargetUnit->LastCommittedCoordinates = TargetUnit->GetCurrentCoordinates();
		TargetUnit->MovementRangeTiles.Empty();
	}

	DealOwnerSkillDamage(TargetUnit);
}

float ULFPSkill_Charge::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return DamageScale;
}

ELFPAttackType ULFPSkill_Charge::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return ELFPAttackType::AT_Physical;
}

bool ULFPSkill_Charge::GetPushDestination(ALFPHexTile* TargetTile, ALFPHexTile*& OutDestinationTile) const
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

	const FLFPHexCoordinates Direction(TargetCoord.Q - OwnerCoord.Q, TargetCoord.R - OwnerCoord.R);
	const FLFPHexCoordinates DestinationCoord(TargetCoord.Q + Direction.Q, TargetCoord.R + Direction.R);
	ALFPHexTile* DestinationTile = GridManager->GetTileAtCoordinates(DestinationCoord);
	if (!DestinationTile || !DestinationTile->IsWalkable() || DestinationTile->IsOccupied() || DestinationTile->GetUnitOnTile())
	{
		return false;
	}

	OutDestinationTile = DestinationTile;
	return true;
}
