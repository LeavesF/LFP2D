#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"

bool ULFPBuffDefinitionDataAsset::AreConditionsMet(const FLFPBuffConditionContext& Context) const
{
    if (Conditions.IsEmpty())
    {
        return true;
    }

    if (ConditionMatchType == ELFPBuffConditionMatchType::Any)
    {
        for (const ULFPBuffCondition* Condition : Conditions)
        {
            if (Condition && Condition->IsMet(Context))
            {
                return true;
            }
        }

        return false;
    }

    for (const ULFPBuffCondition* Condition : Conditions)
    {
        if (Condition && !Condition->IsMet(Context))
        {
            return false;
        }
    }

    return true;
}

void ULFPBuffDefinitionDataAsset::ExecuteEffects(ELFPBuffTriggerEvent TriggerEvent, const FLFPBuffEffectContext& Context) const
{
    for (const ULFPBuffEffect* Effect : Effects)
    {
        if (Effect && Effect->MatchesTrigger(TriggerEvent))
        {
            Effect->Execute(Context);
        }
    }
}

FLFPBuffStatModifier ULFPBuffDefinitionDataAsset::GetStatModifier(const FLFPBuffEffectContext& Context) const
{
    FLFPBuffStatModifier CombinedModifier;

    for (const ULFPBuffEffect* Effect : Effects)
    {
        if (!Effect || !Effect->MatchesTrigger(ELFPBuffTriggerEvent::PassiveStat))
        {
            continue;
        }

        CombinedModifier.Append(Effect->GetStatModifier(Context));
    }

    return CombinedModifier;
}
