#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPBuffEffect::Execute_Implementation(const FLFPBuffEffectContext& Context) const
{
}

FLFPBuffStatModifier ULFPBuffEffect::GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const
{
    return FLFPBuffStatModifier();
}

ULFPBuffEffect_PeriodicDamage::ULFPBuffEffect_PeriodicDamage()
{
    TriggerEvent = ELFPBuffTriggerEvent::OnTurnStart;
}

void ULFPBuffEffect_PeriodicDamage::Execute_Implementation(const FLFPBuffEffectContext& Context) const
{
    ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    if (!TargetUnit || !TargetUnit->IsAlive() || Damage <= 0)
    {
        return;
    }

    const int32 StackCount = bScaleByStack ? FMath::Max(Context.StackCount, 1) : 1;
    TargetUnit->TakeTrueDamage(Damage * StackCount);
}

ULFPBuffEffect_ModifyStat::ULFPBuffEffect_ModifyStat()
{
    TriggerEvent = ELFPBuffTriggerEvent::PassiveStat;
}

FLFPBuffStatModifier ULFPBuffEffect_ModifyStat::GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const
{
    FLFPBuffStatModifier Result = StatModifier;
    if (bScaleByStack)
    {
        const int32 StackCount = FMath::Max(Context.StackCount, 1);
        Result.AttackDelta *= StackCount;
        Result.PhysicalBlockDelta *= StackCount;
        Result.SpeedDelta *= StackCount;
    }

    return Result;
}
