#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

namespace
{
constexpr const TCHAR* BleedBuffIdName = TEXT("Buff.Status.Bleed");
constexpr int32 DefaultBleedDurationTurns = 2;

bool IsTagNamed(const FGameplayTag& Tag, const TCHAR* TagName)
{
    return Tag.IsValid() && Tag.ToString().Equals(TagName, ESearchCase::IgnoreCase);
}

bool IsBleedBuffId(const FGameplayTag& Tag)
{
    return IsTagNamed(Tag, BleedBuffIdName);
}

bool IsBleedDefinition(const ULFPBuffDefinitionDataAsset* BuffDefinition)
{
    return BuffDefinition && IsBleedBuffId(BuffDefinition->BuffId);
}

ELFPBuffStackingMode GetEffectiveStackingMode(const ULFPBuffDefinitionDataAsset* BuffDefinition)
{
    if (IsBleedDefinition(BuffDefinition) &&
        BuffDefinition->StackingPolicy.StackingMode == ELFPBuffStackingMode::AddInstance)
    {
        // 流血是项目内建状态：即使数据资产还保持默认 AddInstance，也按层数合并，避免 UI 和伤害结算分裂。
        return ELFPBuffStackingMode::AddStackAndRefreshDuration;
    }

    return BuffDefinition
        ? BuffDefinition->StackingPolicy.StackingMode
        : ELFPBuffStackingMode::AddInstance;
}

ELFPBuffDurationType GetEffectiveDurationType(const ULFPBuffDefinitionDataAsset* BuffDefinition, int32 DurationTurnsOverride)
{
    if (!BuffDefinition)
    {
        return ELFPBuffDurationType::Infinite;
    }

    if (DurationTurnsOverride >= 0)
    {
        return ELFPBuffDurationType::TimedTurns;
    }

    if (IsBleedDefinition(BuffDefinition))
    {
        // 流血默认是有限回合状态；资产未配置 DurationPolicy 时也能显示和递减回合。
        return ELFPBuffDurationType::TimedTurns;
    }

    return BuffDefinition->DurationPolicy.DurationType;
}

int32 GetEffectiveDurationTurns(const ULFPBuffDefinitionDataAsset* BuffDefinition, int32 DurationTurnsOverride)
{
    if (DurationTurnsOverride >= 0)
    {
        return DurationTurnsOverride;
    }

    if (!BuffDefinition)
    {
        return 0;
    }

    const int32 ConfiguredTurns = BuffDefinition->DurationPolicy.DurationTurns;
    if (IsBleedDefinition(BuffDefinition) && ConfiguredTurns <= 0)
    {
        return DefaultBleedDurationTurns;
    }

    return ConfiguredTurns;
}
}

ULFPBuffComponent::ULFPBuffComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULFPBuffComponent::ApplyBuff(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit, int32 InitialStackCount, int32 DurationTurnsOverride)
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive() || !BuffDefinition)
    {
        return;
    }

    AddBuffInstance(BuffDefinition, SourceUnit ? SourceUnit : OwnerUnit, InitialStackCount, DurationTurnsOverride);
    EvaluateBuffs();
}

void ULFPBuffComponent::ClearPassiveBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    bool bRemovedAny = false;
    for (int32 Index = BuffInstances.Num() - 1; Index >= 0; --Index)
    {
        if (!BuffInstances[Index].bIsPersistent)
        {
            continue;
        }

        if (OwnerUnit && OwnerUnit->IsAlive())
        {
            ExecuteBuffEffects(BuffInstances[Index], ELFPBuffTriggerEvent::OnRemove, OwnerUnit);
        }
        BuffInstances.RemoveAt(Index);
        bRemovedAny = true;
    }

    if (bRemovedAny)
    {
        BroadcastBuffListChanged();
    }
}

bool ULFPBuffComponent::EvaluateBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || BuffInstances.IsEmpty())
    {
        return false;
    }

    bool bChanged = false;
    for (FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        const bool bShouldBeConditionMet = EvaluateBuffCondition(BuffInstance, OwnerUnit);
        if (BuffInstance.bIsConditionMet != bShouldBeConditionMet)
        {
            BuffInstance.bIsConditionMet = bShouldBeConditionMet;
            bChanged = true;
        }
    }

    if (bChanged)
    {
        BroadcastBuffListChanged();
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

    EvaluateBuffs();
    const bool bHadBuffs = !BuffInstances.IsEmpty();

    for (int32 Index = 0; Index < BuffInstances.Num(); ++Index)
    {
        if (!OwnerUnit->IsAlive())
        {
            break;
        }

        FLFPBuffInstance& BuffInstance = BuffInstances[Index];
        if (BuffInstance.HasTimedDuration() && BuffInstance.RemainingTurns <= 0)
        {
            continue;
        }

        if (BuffInstance.IsActive())
        {
            ExecuteBuffEffects(BuffInstance, ELFPBuffTriggerEvent::OnTurnStart, OwnerUnit);
        }

        if (!OwnerUnit->IsAlive())
        {
            break;
        }

        if (BuffInstance.HasTimedDuration())
        {
            BuffInstance.RemainingTurns--;
        }
    }

    CleanupExpiredBuffs();
    if (bHadBuffs)
    {
        BroadcastBuffListChanged();
    }
}

void ULFPBuffComponent::OnTurnEnded()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive())
    {
        return;
    }

    EvaluateBuffs();
    const bool bHadBuffs = !BuffInstances.IsEmpty();

    for (int32 Index = 0; Index < BuffInstances.Num(); ++Index)
    {
        if (!OwnerUnit->IsAlive())
        {
            break;
        }

        FLFPBuffInstance& BuffInstance = BuffInstances[Index];
        if (BuffInstance.IsActive())
        {
            ExecuteBuffEffects(BuffInstance, ELFPBuffTriggerEvent::OnTurnEnd, OwnerUnit);
        }
    }

    CleanupExpiredBuffs();
    if (bHadBuffs)
    {
        BroadcastBuffListChanged();
    }
}

int32 ULFPBuffComponent::RemoveBuffById(FGameplayTag BuffId)
{
    if (!BuffId.IsValid())
    {
        return 0;
    }

    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    int32 RemovedCount = 0;
    for (int32 Index = BuffInstances.Num() - 1; Index >= 0; --Index)
    {
        if (BuffInstances[Index].BuffId != BuffId)
        {
            continue;
        }

        if (OwnerUnit && OwnerUnit->IsAlive())
        {
            ExecuteBuffEffects(BuffInstances[Index], ELFPBuffTriggerEvent::OnRemove, OwnerUnit);
        }
        BuffInstances.RemoveAt(Index);
        RemovedCount++;
    }

    if (RemovedCount > 0)
    {
        BroadcastBuffListChanged();
    }

    return RemovedCount;
}

void ULFPBuffComponent::ClearAllBuffs()
{
    const bool bHadBuffs = !BuffInstances.IsEmpty();
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (OwnerUnit && OwnerUnit->IsAlive())
    {
        for (FLFPBuffInstance& BuffInstance : BuffInstances)
        {
            ExecuteBuffEffects(BuffInstance, ELFPBuffTriggerEvent::OnRemove, OwnerUnit);
        }
    }

    BuffInstances.Empty();
    if (bHadBuffs)
    {
        BroadcastBuffListChanged();
    }
}

bool ULFPBuffComponent::HasAnyBuffs() const
{
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive())
        {
            return true;
        }
    }

    return false;
}

bool ULFPBuffComponent::HasBuffById(FGameplayTag BuffId) const
{
    if (!BuffId.IsValid())
    {
        return false;
    }

    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive() && BuffInstance.BuffId == BuffId)
        {
            return true;
        }
    }

    return false;
}

int32 ULFPBuffComponent::GetBuffStack(FGameplayTag BuffId) const
{
    if (!BuffId.IsValid())
    {
        return 0;
    }

    int32 StackCount = 0;
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive() && BuffInstance.BuffId == BuffId)
        {
            StackCount += FMath::Max(BuffInstance.StackCount, 1);
        }
    }

    return StackCount;
}

int32 ULFPBuffComponent::GetTotalBuffCount() const
{
    int32 BuffCount = 0;
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive())
        {
            BuffCount++;
        }
    }

    return BuffCount;
}

FLFPBuffStatModifier ULFPBuffComponent::GetActiveStatModifier() const
{
    FLFPBuffStatModifier CombinedModifier;
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();

    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (!BuffInstance.IsActive() || !BuffInstance.Definition)
        {
            continue;
        }

        const FLFPBuffEffectContext Context = MakeEffectContext(BuffInstance, OwnerUnit);
        CombinedModifier.Append(BuffInstance.Definition->GetStatModifier(Context));
    }

    return CombinedModifier;
}

float ULFPBuffComponent::GetOutgoingDamageMultiplier() const
{
    return FMath::Max(0.0f, GetActiveStatModifier().OutgoingDamageMultiplier);
}

TArray<FLFPBuffDisplayEntry> ULFPBuffComponent::GetBuffDisplayEntries(bool bOnlyVisible, bool bOnlyActive) const
{
    TArray<FLFPBuffDisplayEntry> Entries;
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        const FLFPBuffDisplayEntry Entry = MakeDisplayEntryFromInstance(BuffInstance);
        if (bOnlyVisible && !Entry.bVisibleInUI)
        {
            continue;
        }

        if (bOnlyActive && !Entry.bIsActive)
        {
            continue;
        }

        Entries.Add(Entry);
    }

    return Entries;
}

TArray<FLFPBuffDisplayEntry> ULFPBuffComponent::GetVisibleBuffDisplayEntries() const
{
    return GetBuffDisplayEntries(true, true);
}

TArray<FLFPBuffDisplayEntry> ULFPBuffComponent::GetAggregatedVisibleBuffDisplayEntries() const
{
    TArray<FLFPBuffDisplayEntry> AggregatedEntries;
    for (const FLFPBuffDisplayEntry& Entry : GetVisibleBuffDisplayEntries())
    {
        int32 ExistingIndex = INDEX_NONE;
        if (Entry.BuffId.IsValid())
        {
            for (int32 Index = 0; Index < AggregatedEntries.Num(); ++Index)
            {
                if (AggregatedEntries[Index].BuffId == Entry.BuffId)
                {
                    ExistingIndex = Index;
                    break;
                }
            }
        }

        if (ExistingIndex == INDEX_NONE)
        {
            AggregatedEntries.Add(Entry);
            continue;
        }

        FLFPBuffDisplayEntry& ExistingEntry = AggregatedEntries[ExistingIndex];
        ExistingEntry.InstanceId.Reset();
        ExistingEntry.StackCount = FMath::Max(ExistingEntry.StackCount, 1) + FMath::Max(Entry.StackCount, 1);
        ExistingEntry.RemainingTurns = FMath::Max(ExistingEntry.RemainingTurns, Entry.RemainingTurns);
        ExistingEntry.bIsActive = ExistingEntry.bIsActive || Entry.bIsActive;

        if (!ExistingEntry.Icon && Entry.Icon)
        {
            ExistingEntry.Icon = Entry.Icon;
        }

        if (ExistingEntry.Description.IsEmpty() && !Entry.Description.IsEmpty())
        {
            ExistingEntry.Description = Entry.Description;
        }
    }

    return AggregatedEntries;
}

ALFPTacticsUnit* ULFPBuffComponent::GetOwnerUnit() const
{
    return Cast<ALFPTacticsUnit>(GetOwner());
}

void ULFPBuffComponent::AddBuffInstance(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit, int32 InitialStackCount, int32 DurationTurnsOverride)
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive() || !BuffDefinition)
    {
        return;
    }

    const int32 SafeInitialStackCount = FMath::Max(InitialStackCount, 1);
    const ELFPBuffStackingMode EffectiveStackingMode = GetEffectiveStackingMode(BuffDefinition);
    const bool bCanStackById = BuffDefinition->BuffId.IsValid() &&
        EffectiveStackingMode != ELFPBuffStackingMode::AddInstance;

    if (bCanStackById)
    {
        for (int32 Index = BuffInstances.Num() - 1; Index >= 0; --Index)
        {
            FLFPBuffInstance& ExistingInstance = BuffInstances[Index];
            if (ExistingInstance.BuffId != BuffDefinition->BuffId)
            {
                continue;
            }

            switch (EffectiveStackingMode)
            {
            case ELFPBuffStackingMode::RefreshDuration:
                RefreshInstanceDuration(ExistingInstance, DurationTurnsOverride);
                BroadcastBuffListChanged();
                return;

            case ELFPBuffStackingMode::AddStackAndRefreshDuration:
                ExistingInstance.StackCount = FMath::Clamp(
                    ExistingInstance.StackCount + SafeInitialStackCount,
                    1,
                    FMath::Max(BuffDefinition->StackingPolicy.MaxStacks, 1));
                RefreshInstanceDuration(ExistingInstance, DurationTurnsOverride);
                BroadcastBuffListChanged();
                return;

            case ELFPBuffStackingMode::Replace:
                if (OwnerUnit->IsAlive())
                {
                    ExecuteBuffEffects(ExistingInstance, ELFPBuffTriggerEvent::OnRemove, OwnerUnit);
                }
                BuffInstances.RemoveAt(Index);
                break;

            case ELFPBuffStackingMode::AddInstance:
            default:
                break;
            }
        }
    }

    FLFPBuffInstance RuntimeInstance;
    RuntimeInstance.Definition = BuffDefinition;
    RuntimeInstance.BuffId = BuffDefinition->BuffId;
    RuntimeInstance.SourceUnit = SourceUnit ? SourceUnit : OwnerUnit;
    RuntimeInstance.TargetUnit = OwnerUnit;
    RuntimeInstance.DurationType = GetEffectiveDurationType(BuffDefinition, DurationTurnsOverride);
    RuntimeInstance.StackCount = FMath::Clamp(
        SafeInitialStackCount,
        1,
        FMath::Max(BuffDefinition->StackingPolicy.MaxStacks, 1));
    RuntimeInstance.bIsPersistent = RuntimeInstance.DurationType != ELFPBuffDurationType::TimedTurns;
    RuntimeInstance.InstanceId = FGuid::NewGuid();
    RefreshInstanceDuration(RuntimeInstance, DurationTurnsOverride);
    RuntimeInstance.bIsConditionMet = EvaluateBuffCondition(RuntimeInstance, OwnerUnit);

    const int32 AddedIndex = BuffInstances.Add(RuntimeInstance);
    if (BuffInstances.IsValidIndex(AddedIndex) && BuffInstances[AddedIndex].IsActive())
    {
        ExecuteBuffEffects(BuffInstances[AddedIndex], ELFPBuffTriggerEvent::OnApply, OwnerUnit);
    }

    BroadcastBuffListChanged();
}

void ULFPBuffComponent::ExecuteBuffEffects(FLFPBuffInstance& BuffInstance, ELFPBuffTriggerEvent TriggerEvent, ALFPTacticsUnit* OwnerUnit)
{
    if (!OwnerUnit || !BuffInstance.Definition)
    {
        return;
    }

    const FLFPBuffEffectContext Context = MakeEffectContext(BuffInstance, OwnerUnit);
    BuffInstance.Definition->ExecuteEffects(TriggerEvent, Context);
}

void ULFPBuffComponent::CleanupExpiredBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    for (int32 Index = BuffInstances.Num() - 1; Index >= 0; --Index)
    {
        if (!BuffInstances[Index].IsExpired())
        {
            continue;
        }

        if (OwnerUnit && OwnerUnit->IsAlive())
        {
            ExecuteBuffEffects(BuffInstances[Index], ELFPBuffTriggerEvent::OnRemove, OwnerUnit);
        }
        BuffInstances.RemoveAt(Index);
    }
}

bool ULFPBuffComponent::EvaluateBuffCondition(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const
{
    if (!OwnerUnit || !OwnerUnit->IsAlive() || !BuffInstance.Definition)
    {
        return false;
    }

    return BuffInstance.Definition->AreConditionsMet(MakeConditionContext(BuffInstance, OwnerUnit));
}

FLFPBuffEffectContext ULFPBuffComponent::MakeEffectContext(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const
{
    FLFPBuffEffectContext Context;
    Context.TargetUnit = BuffInstance.TargetUnit ? BuffInstance.TargetUnit.Get() : OwnerUnit;
    Context.SourceUnit = BuffInstance.SourceUnit ? BuffInstance.SourceUnit.Get() : OwnerUnit;
    Context.BuffId = BuffInstance.BuffId;
    Context.StackCount = BuffInstance.StackCount;
    Context.RemainingTurns = BuffInstance.RemainingTurns;
    return Context;
}

FLFPBuffConditionContext ULFPBuffComponent::MakeConditionContext(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const
{
    FLFPBuffConditionContext Context;
    Context.TargetUnit = BuffInstance.TargetUnit ? BuffInstance.TargetUnit.Get() : OwnerUnit;
    Context.SourceUnit = BuffInstance.SourceUnit ? BuffInstance.SourceUnit.Get() : OwnerUnit;
    Context.BuffId = BuffInstance.BuffId;
    Context.StackCount = BuffInstance.StackCount;
    Context.RemainingTurns = BuffInstance.RemainingTurns;
    return Context;
}

bool ULFPBuffComponent::IsBleedInstance(const FLFPBuffInstance& BuffInstance) const
{
    return IsBleedBuffId(BuffInstance.BuffId);
}

FLFPBuffDisplayEntry ULFPBuffComponent::MakeDisplayEntryFromInstance(const FLFPBuffInstance& BuffInstance) const
{
    FLFPBuffDisplayEntry Entry;
    Entry.BuffId = BuffInstance.BuffId;
    Entry.InstanceId = BuffInstance.InstanceId.IsValid() ? BuffInstance.InstanceId.ToString() : FString();
    Entry.StackCount = FMath::Max(BuffInstance.StackCount, 1);
    Entry.RemainingTurns = BuffInstance.RemainingTurns;
    Entry.bIsActive = BuffInstance.IsActive();

    if (BuffInstance.Definition)
    {
        Entry.DisplayName = BuffInstance.Definition->DisplayName;
        Entry.Description = BuffInstance.Definition->Description;
        Entry.Icon = BuffInstance.Definition->Icon;
        Entry.Category = BuffInstance.Definition->Category;
        Entry.bVisibleInUI = BuffInstance.Definition->bVisibleInUI;
    }

    if (Entry.DisplayName.IsEmpty())
    {
        if (BuffInstance.BuffId.IsValid())
        {
            Entry.DisplayName = FText::FromString(BuffInstance.BuffId.ToString());
        }
        else
        {
            Entry.DisplayName = FText::FromString(TEXT("Buff"));
        }
    }

    return Entry;
}

void ULFPBuffComponent::RefreshInstanceDuration(FLFPBuffInstance& BuffInstance, int32 DurationTurnsOverride) const
{
    if (!BuffInstance.Definition)
    {
        BuffInstance.RemainingTurns = 0;
        return;
    }

    BuffInstance.DurationType = GetEffectiveDurationType(BuffInstance.Definition, DurationTurnsOverride);
    if (BuffInstance.HasTimedDuration())
    {
        BuffInstance.RemainingTurns = GetEffectiveDurationTurns(BuffInstance.Definition, DurationTurnsOverride);
    }
    else
    {
        BuffInstance.RemainingTurns = 0;
    }
}

void ULFPBuffComponent::BroadcastBuffListChanged()
{
    OnBuffListChanged.Broadcast();
}
