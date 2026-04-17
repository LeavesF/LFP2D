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
}

bool ULFPBuffComponent::HasAnyBuffs() const
{
    return ActiveBuffs.Num() > 0;
}

bool ULFPBuffComponent::HasBuff(ELFPBuffType BuffType) const
{
    return ActiveBuffs.ContainsByPredicate([BuffType](const FLFPActiveBuff& Buff)
    {
        return Buff.BuffType == BuffType && Buff.RemainingTurnTriggers > 0;
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
    return BuffCount;
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
    return BuffCount;
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
