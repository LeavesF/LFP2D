#include "LFP2D/Skill/SkillInstance/LFPSkill_WoundBite.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_WoundBite::ULFPSkill_WoundBite()
{
    SkillName = FText::FromString(TEXT("伤口撕咬"));
    SkillDescription = FText::FromString(TEXT("选中一个相邻敌方单位，造成一次目标当前流血总层数×3的伤害"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
}

bool ULFPSkill_WoundBite::CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
    if (!Super::CanPlanFrom_Implementation(CasterTile, TargetTile))
    {
        return false;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit || !TargetUnit->IsAlive() || !IsHostileTarget(TargetUnit))
    {
        return false;
    }

    const ULFPBuffComponent* BuffComponent = TargetUnit->GetBuffComponent();
    return BuffComponent && BuffComponent->GetBleedStacks() > 0;
}

bool ULFPSkill_WoundBite::CanExecute_Implementation(ALFPHexTile* TargetTile)
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
    if (!OwnerTile || !CanReleaseFrom(OwnerTile, TargetTile))
    {
        return false;
    }

    const ULFPBuffComponent* BuffComponent = TargetUnit->GetBuffComponent();
    return BuffComponent && BuffComponent->GetBleedStacks() > 0;
}

void ULFPSkill_WoundBite::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile))
    {
        return;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit || !Owner)
    {
        return;
    }

    const ULFPBuffComponent* BuffComponent = TargetUnit->GetBuffComponent();
    if (!BuffComponent)
    {
        return;
    }

    const int32 BleedStacks = BuffComponent->GetBleedStacks();
    if (BleedStacks <= 0)
    {
        return;
    }

    const int32 Damage = BleedStacks * DamagePerBleedStack;
    TargetUnit->TakeTypedDamage(Damage, Owner->GetAttackType());
}
