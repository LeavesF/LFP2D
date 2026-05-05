#include "LFP2D/Buff/LFPBuffCondition.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

bool ULFPBuffCondition::IsMet_Implementation(const FLFPBuffConditionContext& Context) const
{
    return true;
}

bool ULFPBuffCondition_NoFriendlyWithinRange::IsMet_Implementation(const FLFPBuffConditionContext& Context) const
{
    const ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    return TargetUnit && TargetUnit->IsAlive() && !TargetUnit->HasAliveFriendlyWithinHexRange(Range, bExcludeSelf);
}

bool ULFPBuffCondition_HealthPercentBelow::IsMet_Implementation(const FLFPBuffConditionContext& Context) const
{
    const ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    if (!TargetUnit || !TargetUnit->IsAlive() || TargetUnit->GetCurrentMaxHealth() <= 0)
    {
        return false;
    }

    const float HealthPercent = static_cast<float>(TargetUnit->GetCurrentHealth()) / TargetUnit->GetCurrentMaxHealth();
    return HealthPercent < Threshold;
}

bool ULFPBuffCondition_HasBuff::IsMet_Implementation(const FLFPBuffConditionContext& Context) const
{
    const ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    const ULFPBuffComponent* BuffComponent = TargetUnit ? TargetUnit->GetBuffComponent() : nullptr;
    const bool bHasBuff = BuffComponent && BuffComponent->HasBuffById(RequiredBuffId);
    return bInvert ? !bHasBuff : bHasBuff;
}

bool ULFPBuffCondition_HasGameplayTag::IsMet_Implementation(const FLFPBuffConditionContext& Context) const
{
    const ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    const bool bHasTag = TargetUnit && TargetUnit->HasSpecialTag(RequiredTag);
    return bInvert ? !bHasTag : bHasTag;
}
