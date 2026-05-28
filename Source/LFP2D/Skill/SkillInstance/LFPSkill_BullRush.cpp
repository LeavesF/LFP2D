#include "LFP2D/Skill/SkillInstance/LFPSkill_BullRush.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_BullRush::ULFPSkill_BullRush()
{
	SkillName = FText::FromString(TEXT("蛮牛冲撞"));
	SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，自己本回合移动了几格，就将目标沿远离释放者的方向撞动几格，并造成一次 100% 攻击力的物理伤害。目标撞到不可行走格时会停下。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::SingleEnemy;
	ReleaseRangeType = ESkillRangeType::Coverage;
	MaxRange = 1;
	EffectRangeType = ESkillRangeType::Origin;
	bTriggersEnemyMissBuffOnMiss = true;
}

bool ULFPSkill_BullRush::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_BullRush::Execute_Implementation(ALFPHexTile* TargetTile)
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

	FLFPHexCoordinates Direction;
	if (GetRushDirection(TargetTile, Direction))
	{
		const int32 PushDistance = Owner->GetMovedHexDistanceThisRound();
		if (ALFPHexTile* DestinationTile = GetBullRushDestination(TargetTile, Direction, PushDistance))
		{
			if (DestinationTile != TargetTile)
			{
				TargetUnit->SetCurrentCoordinates(DestinationTile->GetCoordinates(), true);
				TargetUnit->LastCommittedCoordinates = TargetUnit->GetCurrentCoordinates();
				TargetUnit->MovementRangeTiles.Empty();
			}
		}
	}

	DealOwnerSkillDamage(TargetUnit);
}

float ULFPSkill_BullRush::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return DamageScale;
}

ELFPAttackType ULFPSkill_BullRush::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return ELFPAttackType::AT_Physical;
}

bool ULFPSkill_BullRush::GetRushDirection(ALFPHexTile* TargetTile, FLFPHexCoordinates& OutDirection) const
{
	if (!Owner || !TargetTile)
	{
		return false;
	}

	const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
	const FLFPHexCoordinates TargetCoord = TargetTile->GetCoordinates();
	if (FLFPHexCoordinates::Distance(OwnerCoord, TargetCoord) != 1)
	{
		return false;
	}

	OutDirection = FLFPHexCoordinates(TargetCoord.Q - OwnerCoord.Q, TargetCoord.R - OwnerCoord.R);
	for (const FLFPHexCoordinates& Direction : ALFPHexGridManager::HexDirections)
	{
		if (OutDirection == Direction)
		{
			return true;
		}
	}

	return false;
}

ALFPHexTile* ULFPSkill_BullRush::GetBullRushDestination(
	ALFPHexTile* TargetTile,
	const FLFPHexCoordinates& Direction,
	int32 PushDistance) const
{
	if (!Owner || !TargetTile || PushDistance <= 0)
	{
		return TargetTile;
	}

	ALFPHexGridManager* GridManager = Owner->GetGridManager();
	if (!GridManager)
	{
		return TargetTile;
	}

	ALFPHexTile* LastValidTile = TargetTile;
	FLFPHexCoordinates CurrentCoord = TargetTile->GetCoordinates();
	for (int32 Step = 0; Step < PushDistance; ++Step)
	{
		const FLFPHexCoordinates NextCoord(CurrentCoord.Q + Direction.Q, CurrentCoord.R + Direction.R);
		ALFPHexTile* NextTile = GridManager->GetTileAtCoordinates(NextCoord);
		if (!NextTile || !NextTile->IsWalkable() || NextTile->IsOccupied() || NextTile->GetUnitOnTile())
		{
			break;
		}

		LastValidTile = NextTile;
		CurrentCoord = NextCoord;
	}

	return LastValidTile;
}
