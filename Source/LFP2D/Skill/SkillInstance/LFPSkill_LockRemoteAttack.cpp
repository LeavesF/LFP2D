#include "LFP2D/Skill/SkillInstance/LFPSkill_LockRemoteAttack.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_LockRemoteAttack::ULFPSkill_LockRemoteAttack()
{
    SkillName = FText::FromString(TEXT("锁头远程攻击"));
    SkillDescription = FText::FromString(TEXT("对2到5格内的单个敌方单位造成一次100%攻击力的物理伤害。敌方AI使用时会在释放瞬间锁定目标当前所在位置。"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Ring;
    MinRange = 2;
    MaxRange = 5;
    EffectRangeType = ESkillRangeType::Origin;
    bTrackTargetUnitForAIExecution = true;
}

bool ULFPSkill_LockRemoteAttack::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_LockRemoteAttack::Execute_Implementation(ALFPHexTile* TargetTile)
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

float ULFPSkill_LockRemoteAttack::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
    return DamageScale;
}

ELFPAttackType ULFPSkill_LockRemoteAttack::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
    return ELFPAttackType::AT_Physical;
}
