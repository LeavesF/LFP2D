#include "LFP2D/Skill/SkillInstance/LFPSkill_BasicMeleeAttack.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_BasicMeleeAttack::ULFPSkill_BasicMeleeAttack()
{
    SkillName = FText::FromString(TEXT("基本近战攻击"));
    SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，造成一次100%攻击力的物理伤害"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
    bIsDefaultAttack = true;
}

bool ULFPSkill_BasicMeleeAttack::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_BasicMeleeAttack::Execute_Implementation(ALFPHexTile* TargetTile)
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

float ULFPSkill_BasicMeleeAttack::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
    return DamageScale;
}

ELFPAttackType ULFPSkill_BasicMeleeAttack::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
    return ELFPAttackType::AT_Physical;
}
