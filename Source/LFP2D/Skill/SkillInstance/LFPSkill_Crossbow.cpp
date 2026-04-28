#include "LFP2D/Skill/SkillInstance/LFPSkill_Crossbow.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_Crossbow::ULFPSkill_Crossbow()
{
	SkillName = FText::FromString(TEXT("弩箭"));
	SkillDescription = FText::FromString(TEXT("向 6 个方向之一射出弩箭，对射线上的第一个单位造成 150% 攻击力的物理伤害。弩箭会被地形障碍和任何单位阻挡。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::SingleUnit;
	ReleaseRangeType = ESkillRangeType::Ray;
	RayRange = 5;
	EffectRangeType = ESkillRangeType::Origin;
	bRequireLineOfSight = true;
	bStopOnBlocker = true;
	bTrackTargetUnitForAIExecution = true;
}

bool ULFPSkill_Crossbow::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile)
	{
		return false;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	if (!TargetUnit || !TargetUnit->IsAlive())
	{
		return false;
	}

	// SingleUnit 不检查敌对，任何单位都能被击中

	ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
	if (!OwnerTile)
	{
		return false;
	}

	return CanReleaseFrom(OwnerTile, TargetTile);
}

void ULFPSkill_Crossbow::Execute_Implementation(ALFPHexTile* TargetTile)
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

	DealOwnerSkillDamage(TargetUnit);
}

float ULFPSkill_Crossbow::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return DamageScale;
}

ELFPAttackType ULFPSkill_Crossbow::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return ELFPAttackType::AT_Physical;
}
