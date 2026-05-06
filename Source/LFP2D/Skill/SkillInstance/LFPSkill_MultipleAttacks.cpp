#include "LFP2D/Skill/SkillInstance/LFPSkill_MultipleAttacks.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_MultipleAttacks::ULFPSkill_MultipleAttacks()
{
    SkillName = FText::FromString(TEXT("快速连击"));
    SkillDescription = FText::FromString(TEXT("对相邻敌方单位造成多段伤害，每段独立结算减伤。"));
    ActionPointCost = 1;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
    bTriggersEnemyMissBuffOnMiss = true;
}

bool ULFPSkill_MultipleAttacks::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

    if (!IsHostileTarget(TargetUnit))
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

void ULFPSkill_MultipleAttacks::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile))
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

int32 ULFPSkill_MultipleAttacks::GetHitCount_Implementation(ALFPTacticsUnit* Target) const
{
    return Owner ? (2 + Owner->GetAttackCount()) : 0;
}

float ULFPSkill_MultipleAttacks::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
    return DamageScalePerHit;
}
