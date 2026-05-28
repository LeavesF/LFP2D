#include "LFP2D/Skill/SkillInstance/LFPSkill_FlyingSpiritArrow.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_FlyingSpiritArrow::ULFPSkill_FlyingSpiritArrow()
{
	SkillName = FText::FromString(TEXT("飞灵箭"));
	SkillDescription = FText::FromString(TEXT("向 6 个方向之一射出飞灵箭，对射线上的第一个单位造成 100% 攻击力的物理伤害。若此次攻击击杀该单位，释放者位移到该单位所在格。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::AnyTile;
	ReleaseRangeType = ESkillRangeType::Ray;
	RayRange = 5;
	EffectRangeType = ESkillRangeType::Origin;
	bRequireLineOfSight = true;
	bStopOnBlocker = true;
	bTrackTargetUnitForAIExecution = true;
	bTriggersEnemyMissBuffOnMiss = true;
}

bool ULFPSkill_FlyingSpiritArrow::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile)
	{
		return false;
	}

	ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
	if (!OwnerTile)
	{
		return false;
	}

	return CanReleaseFrom(OwnerTile, TargetTile);
}

void ULFPSkill_FlyingSpiritArrow::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile) || !Owner)
	{
		return;
	}

	FLFPHexCoordinates Direction;
	if (!GetRayDirectionToTarget(TargetTile, Direction))
	{
		return;
	}

	ALFPHexTile* HitTile = nullptr;
	ALFPTacticsUnit* HitUnit = FindFirstAliveUnitOnRay(Direction, HitTile);
	if (!HitUnit || !HitTile)
	{
		return;
	}

	const bool bWasAlive = HitUnit->IsAlive();
	const int32 DamageDealt = DealOwnerSkillDamage(HitUnit);
	if (DamageDealt > 0 && bWasAlive && !HitUnit->IsAlive())
	{
		MoveOwnerToTile(HitTile);
	}
}

float ULFPSkill_FlyingSpiritArrow::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return DamageScale;
}

ELFPAttackType ULFPSkill_FlyingSpiritArrow::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return ELFPAttackType::AT_Physical;
}

bool ULFPSkill_FlyingSpiritArrow::GetRayDirectionToTarget(ALFPHexTile* TargetTile, FLFPHexCoordinates& OutDirection) const
{
	if (!Owner || !TargetTile)
	{
		return false;
	}

	const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
	const FLFPHexCoordinates TargetCoord = TargetTile->GetCoordinates();
	const FLFPHexCoordinates RelativeCoord(TargetCoord.Q - OwnerCoord.Q, TargetCoord.R - OwnerCoord.R);
	const int32 Distance = FLFPHexCoordinates::Distance(OwnerCoord, TargetCoord);
	if (Distance <= 0)
	{
		return false;
	}

	if (RelativeCoord.Q % Distance != 0 || RelativeCoord.R % Distance != 0)
	{
		return false;
	}

	OutDirection = FLFPHexCoordinates(RelativeCoord.Q / Distance, RelativeCoord.R / Distance);
	for (const FLFPHexCoordinates& ValidDirection : ALFPHexGridManager::HexDirections)
	{
		if (OutDirection == ValidDirection)
		{
			return true;
		}
	}

	return false;
}

ALFPTacticsUnit* ULFPSkill_FlyingSpiritArrow::FindFirstAliveUnitOnRay(
	const FLFPHexCoordinates& Direction,
	ALFPHexTile*& OutHitTile) const
{
	OutHitTile = nullptr;

	if (!Owner)
	{
		return nullptr;
	}

	ALFPHexGridManager* GridManager = Owner->GetGridManager();
	if (!GridManager)
	{
		return nullptr;
	}

	const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
	const TArray<ALFPHexTile*> RayTiles = GridManager->WalkRayUntilBlocked(OwnerCoord, Direction, RayRange);
	for (ALFPHexTile* Tile : RayTiles)
	{
		if (!Tile)
		{
			continue;
		}

		ALFPTacticsUnit* Unit = Tile->GetUnitOnTile();
		if (Unit && Unit->IsAlive())
		{
			OutHitTile = Tile;
			return Unit;
		}
	}

	return nullptr;
}

void ULFPSkill_FlyingSpiritArrow::MoveOwnerToTile(ALFPHexTile* DestinationTile) const
{
	if (!Owner || !DestinationTile || DestinationTile->GetUnitOnTile())
	{
		return;
	}

	Owner->SetCurrentCoordinates(DestinationTile->GetCoordinates(), true);
	Owner->LastCommittedCoordinates = Owner->GetCurrentCoordinates();
	Owner->MovementRangeTiles.Empty();

	if (ALFPTurnManager* TurnManager = Owner->GetTurnManager())
	{
		TurnManager->RefreshAllRuntimeUnitStates();
	}
}
