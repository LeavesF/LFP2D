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

    // 流血现在也走统一 BuffDefinition，只是效果列表里挂一个回合开始触发的伤害效果。
    FLFPBuffDefinition BuffDefinition;
    BuffDefinition.BuffType = ELFPBuffType::BT_Bleed;
    BuffDefinition.LifetimeType = ELFPBuffLifetimeType::BLT_TimedTurns;
    BuffDefinition.DurationTurns = DurationTurns;

    FLFPBuffEffectSpec EffectSpec;
    EffectSpec.EffectType = ELFPBuffEffectType::BET_PeriodicDamage;
    EffectSpec.TriggerType = ELFPBuffTriggerType::BTT_OnTurnStart;
    EffectSpec.Magnitude = BleedStacks;
    BuffDefinition.Effects.Add(EffectSpec);

    RegisterBuff(BuffDefinition);
}

void ULFPBuffComponent::RegisterBuff(const FLFPBuffDefinition& BuffDefinition)
{
    // 统一入口：不区分临时 Buff 和条件常驻 Buff，差异由 Lifetime/Condition 描述。
    AddBuff(BuffDefinition, !BuffDefinition.HasTimedDuration());
    EvaluateBuffs();
}

void ULFPBuffComponent::RegisterPersistentBuff(const FLFPPersistentBuffDefinition& BuffDefinition)
{
    RegisterBuff(BuffDefinition.ToBuffDefinition());
}

void ULFPBuffComponent::ClearPersistentBuffs()
{
    Buffs.RemoveAll([](const FLFPBuffRuntimeState& BuffState)
    {
        return BuffState.bIsPersistent;
    });
}

bool ULFPBuffComponent::EvaluateBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || Buffs.IsEmpty())
    {
        return false;
    }

    bool bChanged = false;
    for (FLFPBuffRuntimeState& BuffState : Buffs)
    {
        // 条件是否满足是运行时状态，和 Definition 分离，便于统一处理常驻与临时 Buff。
        const bool bShouldBeConditionMet = EvaluateBuffCondition(BuffState.Definition, OwnerUnit);
        if (BuffState.bIsConditionMet != bShouldBeConditionMet)
        {
            BuffState.bIsConditionMet = bShouldBeConditionMet;
            bChanged = true;
        }
    }

    return bChanged;
}

bool ULFPBuffComponent::EvaluatePersistentBuffs()
{
    return EvaluateBuffs();
}

void ULFPBuffComponent::OnTurnStarted()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive())
    {
        return;
    }

    EvaluateBuffs();

    if (Buffs.IsEmpty())
    {
        return;
    }

    for (FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (!OwnerUnit->IsAlive())
        {
            break;
        }

        if (BuffState.Definition.HasTimedDuration() && BuffState.RemainingTurns <= 0)
        {
            continue;
        }

        if (BuffState.IsActive())
        {
            // 先执行本回合开始的效果，再扣减持续回合，保证 N 回合 Buff 能触发 N 次。
            ExecuteBuffEffects(BuffState, ELFPBuffTriggerType::BTT_OnTurnStart, OwnerUnit);
        }

        if (BuffState.Definition.HasTimedDuration())
        {
            BuffState.RemainingTurns--;
        }
    }

    CleanupExpiredBuffs();
}

void ULFPBuffComponent::ClearAllBuffs()
{
    Buffs.Empty();
}

bool ULFPBuffComponent::HasAnyBuffs() const
{
    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.IsActive())
        {
            return true;
        }
    }

    return false;
}

bool ULFPBuffComponent::HasBuff(ELFPBuffType BuffType) const
{
    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.Definition.BuffType == BuffType && BuffState.IsActive())
        {
            return true;
        }
    }

    return false;
}

int32 ULFPBuffComponent::GetBuffCount(ELFPBuffType BuffType) const
{
    int32 BuffCount = 0;
    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.Definition.BuffType == BuffType && BuffState.IsActive())
        {
            BuffCount++;
        }
    }

    return BuffCount;
}

int32 ULFPBuffComponent::GetBleedStacks() const
{
    int32 BleedStacks = 0;

    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.Definition.BuffType != ELFPBuffType::BT_Bleed || !BuffState.IsActive())
        {
            continue;
        }

        const int32 EffectiveStackCount = FMath::Max(BuffState.StackCount, 1);
        for (const FLFPBuffEffectSpec& EffectSpec : BuffState.Definition.Effects)
        {
            if (EffectSpec.EffectType == ELFPBuffEffectType::BET_PeriodicDamage &&
                EffectSpec.TriggerType == ELFPBuffTriggerType::BTT_OnTurnStart)
            {
                BleedStacks += FMath::Max(EffectSpec.Magnitude, 0) * EffectiveStackCount;
            }
        }
    }

    return BleedStacks;
}

int32 ULFPBuffComponent::GetTotalBuffCount() const
{
    int32 BuffCount = 0;
    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.IsActive())
        {
            BuffCount++;
        }
    }

    return BuffCount;
}

FLFPBuffStatModifier ULFPBuffComponent::GetActiveStatModifier() const
{
    FLFPBuffStatModifier CombinedModifier;

    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (!BuffState.IsActive())
        {
            continue;
        }

        const int32 EffectiveStackCount = FMath::Max(BuffState.StackCount, 1);
        for (const FLFPBuffEffectSpec& EffectSpec : BuffState.Definition.Effects)
        {
            if (EffectSpec.EffectType != ELFPBuffEffectType::BET_StatModifier ||
                EffectSpec.TriggerType != ELFPBuffTriggerType::BTT_PassiveStat)
            {
                continue;
            }

            // 属性类效果在重建属性时统一汇总，而不是在 Buff 注册时直接改角色数值。
            FLFPBuffStatModifier WeightedModifier = EffectSpec.StatModifier;
            WeightedModifier.AttackDelta *= EffectiveStackCount;
            WeightedModifier.PhysicalBlockDelta *= EffectiveStackCount;
            WeightedModifier.SpeedDelta *= EffectiveStackCount;
            CombinedModifier.Append(WeightedModifier);
        }
    }

    return CombinedModifier;
}

FLFPBuffStatModifier ULFPBuffComponent::GetActivePersistentStatModifier() const
{
    return GetActiveStatModifier();
}

TArray<FLFPPersistentBuffRuntimeState> ULFPBuffComponent::GetPersistentBuffStates() const
{
    TArray<FLFPPersistentBuffRuntimeState> PersistentStates;

    for (const FLFPBuffRuntimeState& BuffState : Buffs)
    {
        if (BuffState.bIsPersistent)
        {
            PersistentStates.Add(FLFPPersistentBuffRuntimeState::FromRuntimeState(BuffState));
        }
    }

    return PersistentStates;
}

ALFPTacticsUnit* ULFPBuffComponent::GetOwnerUnit() const
{
    return Cast<ALFPTacticsUnit>(GetOwner());
}

void ULFPBuffComponent::AddBuff(const FLFPBuffDefinition& BuffDefinition, bool bIsPersistent)
{
    FLFPBuffRuntimeState RuntimeState;
    RuntimeState.Definition = BuffDefinition;
    RuntimeState.RemainingTurns = BuffDefinition.HasTimedDuration() ? BuffDefinition.DurationTurns : 0;
    // 无条件 Buff 默认激活；带条件的 Buff 会在 EvaluateBuffs 中实时刷新。
    RuntimeState.bIsConditionMet = BuffDefinition.ConditionType == ELFPBuffConditionType::BCT_None;
    RuntimeState.bIsPersistent = bIsPersistent;
    Buffs.Add(RuntimeState);
}

void ULFPBuffComponent::ExecuteBuffEffects(FLFPBuffRuntimeState& BuffState, ELFPBuffTriggerType TriggerType, ALFPTacticsUnit* OwnerUnit)
{
    if (!OwnerUnit)
    {
        return;
    }

    const int32 EffectiveStackCount = FMath::Max(BuffState.StackCount, 1);
    for (const FLFPBuffEffectSpec& EffectSpec : BuffState.Definition.Effects)
    {
        // 一个 Buff 可以挂多个效果，只执行当前触发时机匹配的那部分。
        if (EffectSpec.TriggerType != TriggerType)
        {
            continue;
        }

        switch (EffectSpec.EffectType)
        {
        case ELFPBuffEffectType::BET_PeriodicDamage:
            if (EffectSpec.Magnitude > 0)
            {
                OwnerUnit->TakeTrueDamage(EffectSpec.Magnitude * EffectiveStackCount);
            }
            break;

        case ELFPBuffEffectType::BET_StatModifier:
        default:
            break;
        }

        if (!OwnerUnit->IsAlive())
        {
            break;
        }
    }
}

void ULFPBuffComponent::CleanupExpiredBuffs()
{
    Buffs.RemoveAll([](const FLFPBuffRuntimeState& BuffState)
    {
        return BuffState.IsExpired();
    });
}

bool ULFPBuffComponent::EvaluateBuffCondition(const FLFPBuffDefinition& BuffDefinition, const ALFPTacticsUnit* OwnerUnit) const
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
