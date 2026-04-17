#include "LFP2D/Skill/SkillInstance/LFPSkill_Claw.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_Claw::ULFPSkill_Claw()
{
    SkillName = FText::FromString(TEXT("利爪"));
    SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，造成一次100%攻击力的伤害，并添加5层流血"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
}

bool ULFPSkill_Claw::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_Claw::Execute_Implementation(ALFPHexTile* TargetTile)
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

    DealOwnerRepeatedDamage(TargetUnit, 1, DamageScale);

    if (!TargetUnit->IsAlive())
    {
        return;
    }

    if (ULFPBuffComponent* BuffComponent = TargetUnit->GetBuffComponent())
    {
        BuffComponent->ApplyBleed(BleedStacks, BleedDurationTurns);
    }
}
