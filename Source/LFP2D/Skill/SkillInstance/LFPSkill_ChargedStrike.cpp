#include "LFP2D/Skill/SkillInstance/LFPSkill_ChargedStrike.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_ChargedStrike::ULFPSkill_ChargedStrike()
{
    SkillName = FText::FromString(TEXT("蓄力打击"));
    SkillDescription = FText::FromString(TEXT("选择一个相邻格子进入蓄力，并在下次自己回合开始时对该格当前单位造成300%攻击力伤害。"));
    ActionPointCost = 1;
    TargetType = ESkillTargetType::AnyTile;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
    EffectRangeType = ESkillRangeType::Origin;
}

bool ULFPSkill_ChargedStrike::CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
    if (bHasPendingStrike)
    {
        return false;
    }

    return Super::CanPlanFrom_Implementation(CasterTile, TargetTile);
}

bool ULFPSkill_ChargedStrike::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
    if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile || bHasPendingStrike)
    {
        return false;
    }

    ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
    if (!OwnerTile)
    {
        return false;
    }

    return FLFPHexCoordinates::Distance(OwnerTile->GetCoordinates(), TargetTile->GetCoordinates()) == 1;
}

void ULFPSkill_ChargedStrike::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile))
    {
        return;
    }

    PendingTargetTile = TargetTile;
    bHasPendingStrike = true;
}

void ULFPSkill_ChargedStrike::OnTurnStart()
{
    Super::OnTurnStart();

    if (!bHasPendingStrike)
    {
        return;
    }

    ALFPHexTile* TargetTile = PendingTargetTile.Get();
    PendingTargetTile = nullptr;
    bHasPendingStrike = false;

    if (!Owner || !TargetTile)
    {
        return;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit || !TargetUnit->IsAlive())
    {
        return;
    }

    DealOwnerSkillDamage(TargetUnit);
}

float ULFPSkill_ChargedStrike::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
    return DamageScale;
}
