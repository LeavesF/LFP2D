#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPBuffComponent::ULFPBuffComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULFPBuffComponent::ApplyBleed(int32 BleedStacks, int32 DurationTurns)
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive() || BleedStacks <= 0 || DurationTurns <= 0)
    {
        return;
    }

    FLFPActiveBuff NewBuff;
    NewBuff.BuffType = ELFPBuffType::BT_Bleed;
    NewBuff.DamagePerTrigger = BleedStacks;
    NewBuff.RemainingTurnTriggers = DurationTurns;
    ActiveBuffs.Add(NewBuff);
}

void ULFPBuffComponent::RegisterPersistentBuff(const FLFPPersistentBuffDefinition& BuffDefinition)
{
    FLFPPersistentBuffRuntimeState RuntimeState;
    RuntimeState.Definition = BuffDefinition;
    PersistentBuffs.Add(RuntimeState);
    // 注册后立即评估一次，保证初始化时就拿到正确激活态。
    EvaluatePersistentBuffs();
}

void ULFPBuffComponent::ClearPersistentBuffs()
{
    PersistentBuffs.Empty();
}

bool ULFPBuffComponent::EvaluatePersistentBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit)
    {
        return false;
    }

    bool bChanged = false;
    for (FLFPPersistentBuffRuntimeState& BuffState : PersistentBuffs)
    {
        // 持续 Buff 本身常驻，激活与否完全由条件实时决定。
        const bool bShouldBeActive = EvaluatePersistentBuffCondition(BuffState.Definition, OwnerUnit);
        if (BuffState.bIsActive != bShouldBeActive)
        {
            BuffState.bIsActive = bShouldBeActive;
            bChanged = true;
        }
    }

    return bChanged;
}

void ULFPBuffComponent::OnTurnStarted()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive())
    {
        return;
    }

    for (FLFPActiveBuff& Buff : ActiveBuffs)
    {
        if (!OwnerUnit->IsAlive())
        {
            break;
        }

        if (Buff.RemainingTurnTriggers <= 0)
        {
            continue;
        }

        switch (Buff.BuffType)
        {
        case ELFPBuffType::BT_Bleed:
            OwnerUnit->TakeTrueDamage(Buff.DamagePerTrigger);
            Buff.RemainingTurnTriggers--;
            break;
        default:
            break;
        }
    }

    CleanupExpiredBuffs();
}

void ULFPBuffComponent::ClearAllBuffs()
{
    ActiveBuffs.Empty();
    PersistentBuffs.Empty();
}

bool ULFPBuffComponent::HasAnyBuffs() const
{
    if (!ActiveBuffs.IsEmpty())
    {
        return true;
    }

    return PersistentBuffs.ContainsByPredicate([](const FLFPPersistentBuffRuntimeState& BuffState)
    {
        return BuffState.bIsActive;
    });
}

bool ULFPBuffComponent::HasBuff(ELFPBuffType BuffType) const
{
    if (ActiveBuffs.ContainsByPredicate([BuffType](const FLFPActiveBuff& Buff)
    {
        return Buff.BuffType == BuffType && Buff.RemainingTurnTriggers > 0;
    }))
    {
        return true;
    }

    return PersistentBuffs.ContainsByPredicate([BuffType](const FLFPPersistentBuffRuntimeState& BuffState)
    {
        return BuffState.Definition.BuffType == BuffType && BuffState.bIsActive;
    });
}

int32 ULFPBuffComponent::GetBuffCount(ELFPBuffType BuffType) const
{
    int32 BuffCount = 0;
    for (const FLFPActiveBuff& Buff : ActiveBuffs)
    {
        if (Buff.BuffType == BuffType && Buff.RemainingTurnTriggers > 0)
        {
            BuffCount++;
        }
    }

    for (const FLFPPersistentBuffRuntimeState& BuffState : PersistentBuffs)
    {
        if (BuffState.Definition.BuffType == BuffType && BuffState.bIsActive)
        {
            BuffCount++;
        }
    }

    return BuffCount;
}

int32 ULFPBuffComponent::GetBleedStacks() const
{
    int32 BleedStacks = 0;
    for (const FLFPActiveBuff& Buff : ActiveBuffs)
    {
        if (Buff.BuffType == ELFPBuffType::BT_Bleed && Buff.RemainingTurnTriggers > 0)
        {
            BleedStacks += FMath::Max(Buff.DamagePerTrigger, 0);
        }
    }

    return BleedStacks;
}

int32 ULFPBuffComponent::GetTotalBuffCount() const
{
    int32 BuffCount = 0;
    for (const FLFPActiveBuff& Buff : ActiveBuffs)
    {
        if (Buff.RemainingTurnTriggers > 0)
        {
            BuffCount++;
        }
    }

    for (const FLFPPersistentBuffRuntimeState& BuffState : PersistentBuffs)
    {
        if (BuffState.bIsActive)
        {
            BuffCount++;
        }
    }

    return BuffCount;
}

FLFPBuffStatModifier ULFPBuffComponent::GetActivePersistentStatModifier() const
{
    FLFPBuffStatModifier CombinedModifier;
    for (const FLFPPersistentBuffRuntimeState& BuffState : PersistentBuffs)
    {
        if (BuffState.bIsActive)
        {
            // 当前版本只做平面数值叠加，后续如需百分比层再单独扩展。
            CombinedModifier.Append(BuffState.Definition.StatModifier);
        }
    }

    return CombinedModifier;
}

ALFPTacticsUnit* ULFPBuffComponent::GetOwnerUnit() const
{
    return Cast<ALFPTacticsUnit>(GetOwner());
}

void ULFPBuffComponent::CleanupExpiredBuffs()
{
    ActiveBuffs.RemoveAll([](const FLFPActiveBuff& Buff)
    {
        return Buff.RemainingTurnTriggers <= 0;
    });
}

bool ULFPBuffComponent::EvaluatePersistentBuffCondition(const FLFPPersistentBuffDefinition& BuffDefinition, const ALFPTacticsUnit* OwnerUnit) const
{
    if (!OwnerUnit || !OwnerUnit->IsAlive())
    {
        return false;
    }

    switch (BuffDefinition.ConditionType)
    {
    case ELFPBuffConditionType::BCT_None:
        return true;

    case ELFPBuffConditionType::BCT_NoFriendlyWithinRange:
        return !OwnerUnit->HasAliveFriendlyWithinHexRange(BuffDefinition.ConditionRange, true);

    default:
        return false;
    }
}
