#include "LFP2D/Skill/SkillInstance/LFPSkill_Sharpshooter.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_Sharpshooter::ULFPSkill_Sharpshooter()
{
    SkillName = FText::FromString(TEXT("神射"));
    SkillDescription = FText::FromString(TEXT("对 2 至 7 格内的单个敌人造成一次 100% 攻击力的物理伤害。2 格时 50% 暴击率，3 格时 40%，每远 1 格再降 10%，暴击伤害翻倍。"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Ring;
    MinRange = 2;
    MaxRange = 7;
    EffectRangeType = ESkillRangeType::Origin;
    bTrackTargetUnitForAIExecution = true;
}

bool ULFPSkill_Sharpshooter::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_Sharpshooter::Execute_Implementation(ALFPHexTile* TargetTile)
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

float ULFPSkill_Sharpshooter::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
    return DamageScale;
}

ELFPAttackType ULFPSkill_Sharpshooter::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
    return ELFPAttackType::AT_Physical;
}

float ULFPSkill_Sharpshooter::GetCriticalChance_Implementation(ALFPTacticsUnit* Target) const
{
    if (!Owner || !Target)
    {
        return 0.0f;
    }

    const int32 Distance = FLFPHexCoordinates::Distance(
        Owner->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates());
    return GetCriticalChanceAtDistance(Distance);
}

float ULFPSkill_Sharpshooter::GetCriticalChanceAtDistance(int32 Distance) const
{
    if (Distance < MinRange || Distance > MaxRange)
    {
        return 0.0f;
    }

    // 2 格为基础暴击率，之后每增加 1 格就按固定值衰减。
    const int32 DistanceOffset = Distance - MinRange;
    return FMath::Clamp(
        CloseRangeCriticalChance - (DistanceOffset * CriticalChanceFalloffPerHex),
        0.0f,
        1.0f);
}
