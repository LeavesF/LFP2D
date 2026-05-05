#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffCondition.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

namespace
{
constexpr const TCHAR* BleedBuffIdName = TEXT("Buff.Status.Bleed");
constexpr const TCHAR* LegacyBleedBuffIdName = TEXT("Status.Bleed");
constexpr const TCHAR* StatModifierBuffIdName = TEXT("Buff.StatModifier");
constexpr int32 DefaultBleedDurationTurns = 2;

FGameplayTag RequestOptionalGameplayTag(const TCHAR* TagName)
{
    return FGameplayTag::RequestGameplayTag(FName(TagName), false);
}

FGameplayTag GetLegacyBuffId(ELFPBuffType BuffType)
{
    return BuffType == ELFPBuffType::BT_Bleed
        ? RequestOptionalGameplayTag(BleedBuffIdName)
        : RequestOptionalGameplayTag(StatModifierBuffIdName);
}

bool IsTagNamed(const FGameplayTag& Tag, const TCHAR* TagName)
{
    return Tag.IsValid() && Tag.ToString().Equals(TagName, ESearchCase::IgnoreCase);
}

bool IsBleedBuffId(const FGameplayTag& Tag)
{
    return IsTagNamed(Tag, BleedBuffIdName) || IsTagNamed(Tag, LegacyBleedBuffIdName);
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

int32 NormalizeLegacyBleedDefinition(FLFPBuffDefinition& LegacyDefinition)
{
    if (LegacyDefinition.BuffType != ELFPBuffType::BT_Bleed)
    {
        return 1;
    }

    int32 TotalBleedStacks = 0;
    bool bHasBleedDamageEffect = false;
    for (FLFPBuffEffectSpec& EffectSpec : LegacyDefinition.Effects)
    {
        if (EffectSpec.EffectType != ELFPBuffEffectType::BET_PeriodicDamage ||
            EffectSpec.TriggerType != ELFPBuffTriggerType::BTT_OnTurnStart)
        {
            continue;
        }

        TotalBleedStacks += FMath::Max(EffectSpec.Magnitude, 0);

        // 旧接口里 Magnitude 表示“流血层数”。新运行时统一使用 StackCount 表示层数，所以伤害系数归一为 1。
        EffectSpec.Magnitude = bHasBleedDamageEffect ? 0 : 1;
        bHasBleedDamageEffect = true;
    }

    return FMath::Max(TotalBleedStacks, 1);
}

ELFPBuffDurationType ConvertLegacyDurationType(ELFPBuffLifetimeType LifetimeType)
{
    switch (LifetimeType)
    {
    case ELFPBuffLifetimeType::BLT_TimedTurns:
        return ELFPBuffDurationType::TimedTurns;
    case ELFPBuffLifetimeType::BLT_WhileConditionTrue:
        return ELFPBuffDurationType::WhileConditionTrue;
    case ELFPBuffLifetimeType::BLT_Infinite:
    default:
        return ELFPBuffDurationType::Infinite;
    }
}

ELFPBuffLifetimeType ConvertToLegacyDurationType(ELFPBuffDurationType DurationType)
{
    switch (DurationType)
    {
    case ELFPBuffDurationType::TimedTurns:
        return ELFPBuffLifetimeType::BLT_TimedTurns;
    case ELFPBuffDurationType::WhileConditionTrue:
        return ELFPBuffLifetimeType::BLT_WhileConditionTrue;
    case ELFPBuffDurationType::Infinite:
    default:
        return ELFPBuffLifetimeType::BLT_Infinite;
    }
}

ELFPBuffTriggerEvent ConvertLegacyTriggerType(ELFPBuffTriggerType TriggerType)
{
    switch (TriggerType)
    {
    case ELFPBuffTriggerType::BTT_OnTurnStart:
        return ELFPBuffTriggerEvent::OnTurnStart;
    case ELFPBuffTriggerType::BTT_PassiveStat:
    default:
        return ELFPBuffTriggerEvent::PassiveStat;
    }
}

ELFPBuffTriggerType ConvertToLegacyTriggerType(ELFPBuffTriggerEvent TriggerEvent)
{
    switch (TriggerEvent)
    {
    case ELFPBuffTriggerEvent::OnTurnStart:
        return ELFPBuffTriggerType::BTT_OnTurnStart;
    case ELFPBuffTriggerEvent::PassiveStat:
    default:
        return ELFPBuffTriggerType::BTT_PassiveStat;
    }
}
}

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

void ULFPBuffComponent::ApplyBuff(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit, int32 InitialStackCount, int32 DurationTurnsOverride)
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || !OwnerUnit->IsAlive() || !BuffDefinition)
    {
        return;
    }

    AddBuffInstance(BuffDefinition, SourceUnit ? SourceUnit : OwnerUnit, InitialStackCount, DurationTurnsOverride, false, FLFPBuffDefinition());
    EvaluateBuffs();
}

void ULFPBuffComponent::RegisterBuff(const FLFPBuffDefinition& BuffDefinition)
{
    AddBuff(BuffDefinition, !BuffDefinition.HasTimedDuration());
    EvaluateBuffs();
}

void ULFPBuffComponent::RegisterPersistentBuff(const FLFPPersistentBuffDefinition& BuffDefinition)
{
    RegisterBuff(BuffDefinition.ToBuffDefinition());
}

void ULFPBuffComponent::ClearPersistentBuffs()
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
        SyncLegacyBuffsFromInstances();
        BroadcastBuffListChanged();
    }
}

bool ULFPBuffComponent::EvaluateBuffs()
{
    ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
    if (!OwnerUnit || BuffInstances.IsEmpty())
    {
        SyncLegacyBuffsFromInstances();
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
        SyncLegacyBuffsFromInstances();
        BroadcastBuffListChanged();
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
    SyncLegacyBuffsFromInstances();
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
    SyncLegacyBuffsFromInstances();
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
        SyncLegacyBuffsFromInstances();
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
    Buffs.Empty();
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

bool ULFPBuffComponent::HasBuff(ELFPBuffType BuffType) const
{
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive() && DoesInstanceMatchLegacyType(BuffInstance, BuffType))
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

int32 ULFPBuffComponent::GetBuffCount(ELFPBuffType BuffType) const
{
    int32 BuffCount = 0;
    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive() && DoesInstanceMatchLegacyType(BuffInstance, BuffType))
        {
            BuffCount++;
        }
    }

    return BuffCount;
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

int32 ULFPBuffComponent::GetBleedStacks() const
{
    int32 BleedStacks = 0;

    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        if (BuffInstance.IsActive())
        {
            BleedStacks += GetBleedMagnitudeFromInstance(BuffInstance);
        }
    }

    return BleedStacks;
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

FLFPBuffStatModifier ULFPBuffComponent::GetActivePersistentStatModifier() const
{
    return GetActiveStatModifier();
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

void ULFPBuffComponent::AddBuff(const FLFPBuffDefinition& BuffDefinition, bool /*bIsPersistent*/)
{
    FLFPBuffDefinition LegacyDefinition = BuffDefinition;
    const int32 InitialStackCount = NormalizeLegacyBleedDefinition(LegacyDefinition);
    ULFPBuffDefinitionDataAsset* TransientDefinition = CreateTransientDefinitionFromLegacy(LegacyDefinition);
    if (!TransientDefinition)
    {
        return;
    }

    AddBuffInstance(TransientDefinition, GetOwnerUnit(), InitialStackCount, LegacyDefinition.HasTimedDuration() ? LegacyDefinition.DurationTurns : -1, true, LegacyDefinition);
}

void ULFPBuffComponent::AddBuffInstance(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit, int32 InitialStackCount, int32 DurationTurnsOverride, bool bHasLegacyDefinition, const FLFPBuffDefinition& LegacyDefinition)
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
                SyncLegacyBuffsFromInstances();
                BroadcastBuffListChanged();
                return;

            case ELFPBuffStackingMode::AddStackAndRefreshDuration:
                ExistingInstance.StackCount = FMath::Clamp(
                    ExistingInstance.StackCount + SafeInitialStackCount,
                    1,
                    FMath::Max(BuffDefinition->StackingPolicy.MaxStacks, 1));
                RefreshInstanceDuration(ExistingInstance, DurationTurnsOverride);
                SyncLegacyBuffsFromInstances();
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
    RuntimeInstance.bHasLegacyDefinition = bHasLegacyDefinition;
    RuntimeInstance.LegacyDefinition = LegacyDefinition;
    RefreshInstanceDuration(RuntimeInstance, DurationTurnsOverride);
    RuntimeInstance.bIsConditionMet = EvaluateBuffCondition(RuntimeInstance, OwnerUnit);

    const int32 AddedIndex = BuffInstances.Add(RuntimeInstance);
    if (BuffInstances.IsValidIndex(AddedIndex) && BuffInstances[AddedIndex].IsActive())
    {
        ExecuteBuffEffects(BuffInstances[AddedIndex], ELFPBuffTriggerEvent::OnApply, OwnerUnit);
    }

    SyncLegacyBuffsFromInstances();
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

ULFPBuffDefinitionDataAsset* ULFPBuffComponent::CreateTransientDefinitionFromLegacy(const FLFPBuffDefinition& BuffDefinition)
{
    ULFPBuffDefinitionDataAsset* TransientDefinition = NewObject<ULFPBuffDefinitionDataAsset>(this);
    if (!TransientDefinition)
    {
        return nullptr;
    }

    TransientDefinition->BuffId = GetLegacyBuffId(BuffDefinition.BuffType);
    TransientDefinition->Category = BuffDefinition.BuffType == ELFPBuffType::BT_Bleed
        ? ELFPBuffCategory::Debuff
        : ELFPBuffCategory::Buff;
    TransientDefinition->DurationPolicy.DurationType = ConvertLegacyDurationType(BuffDefinition.LifetimeType);
    TransientDefinition->DurationPolicy.DurationTurns = BuffDefinition.DurationTurns;
    TransientDefinition->StackingPolicy.StackingMode = ELFPBuffStackingMode::AddInstance;
    TransientDefinition->StackingPolicy.MaxStacks = 99;
    TransientDefinition->bVisibleInUI = BuffDefinition.bVisibleInUI;

    if (BuffDefinition.ConditionType == ELFPBuffConditionType::BCT_NoFriendlyWithinRange)
    {
        ULFPBuffCondition_NoFriendlyWithinRange* Condition = NewObject<ULFPBuffCondition_NoFriendlyWithinRange>(TransientDefinition);
        if (Condition)
        {
            Condition->Range = BuffDefinition.ConditionRange;
            TransientDefinition->Conditions.Add(Condition);
        }
    }

    for (const FLFPBuffEffectSpec& EffectSpec : BuffDefinition.Effects)
    {
        switch (EffectSpec.EffectType)
        {
        case ELFPBuffEffectType::BET_PeriodicDamage:
            {
                ULFPBuffEffect_PeriodicDamage* Effect = NewObject<ULFPBuffEffect_PeriodicDamage>(TransientDefinition);
                if (Effect)
                {
                    Effect->TriggerEvent = ConvertLegacyTriggerType(EffectSpec.TriggerType);
                    Effect->Damage = EffectSpec.Magnitude;
                    Effect->bScaleByStack = true;
                    TransientDefinition->Effects.Add(Effect);
                }
            }
            break;

        case ELFPBuffEffectType::BET_StatModifier:
            {
                ULFPBuffEffect_ModifyStat* Effect = NewObject<ULFPBuffEffect_ModifyStat>(TransientDefinition);
                if (Effect)
                {
                    Effect->TriggerEvent = ConvertLegacyTriggerType(EffectSpec.TriggerType);
                    Effect->StatModifier = EffectSpec.StatModifier;
                    Effect->bScaleByStack = true;
                    TransientDefinition->Effects.Add(Effect);
                }
            }
            break;

        default:
            break;
        }
    }

    return TransientDefinition;
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

FLFPBuffRuntimeState ULFPBuffComponent::MakeLegacyStateFromInstance(const FLFPBuffInstance& BuffInstance) const
{
    FLFPBuffRuntimeState RuntimeState;
    RuntimeState.Definition = BuffInstance.bHasLegacyDefinition
        ? BuffInstance.LegacyDefinition
        : FLFPBuffDefinition();

    if (!BuffInstance.bHasLegacyDefinition)
    {
        RuntimeState.Definition.BuffType = IsBleedInstance(BuffInstance)
            ? ELFPBuffType::BT_Bleed
            : ELFPBuffType::BT_StatModifier;
        RuntimeState.Definition.LifetimeType = ConvertToLegacyDurationType(BuffInstance.DurationType);
        RuntimeState.Definition.DurationTurns = BuffInstance.Definition
            ? BuffInstance.Definition->DurationPolicy.DurationTurns
            : 0;
        RuntimeState.Definition.SourceSkillName = BuffInstance.BuffId.IsValid()
            ? FName(*BuffInstance.BuffId.ToString())
            : NAME_None;
        RuntimeState.Definition.bVisibleInUI = BuffInstance.Definition
            ? BuffInstance.Definition->bVisibleInUI
            : true;

        if (BuffInstance.Definition)
        {
            for (const ULFPBuffEffect* Effect : BuffInstance.Definition->Effects)
            {
                if (const ULFPBuffEffect_PeriodicDamage* DamageEffect = Cast<ULFPBuffEffect_PeriodicDamage>(Effect))
                {
                    FLFPBuffEffectSpec EffectSpec;
                    EffectSpec.EffectType = ELFPBuffEffectType::BET_PeriodicDamage;
                    EffectSpec.TriggerType = ConvertToLegacyTriggerType(DamageEffect->TriggerEvent);
                    EffectSpec.Magnitude = DamageEffect->Damage;
                    RuntimeState.Definition.Effects.Add(EffectSpec);
                }
                else if (const ULFPBuffEffect_ModifyStat* StatEffect = Cast<ULFPBuffEffect_ModifyStat>(Effect))
                {
                    FLFPBuffEffectSpec EffectSpec;
                    EffectSpec.EffectType = ELFPBuffEffectType::BET_StatModifier;
                    EffectSpec.TriggerType = ConvertToLegacyTriggerType(StatEffect->TriggerEvent);
                    EffectSpec.StatModifier = StatEffect->StatModifier;
                    RuntimeState.Definition.Effects.Add(EffectSpec);
                }
            }
        }
    }

    RuntimeState.RemainingTurns = BuffInstance.RemainingTurns;
    RuntimeState.StackCount = BuffInstance.StackCount;
    RuntimeState.bIsConditionMet = BuffInstance.bIsConditionMet;
    RuntimeState.bIsPersistent = BuffInstance.bIsPersistent;
    return RuntimeState;
}

bool ULFPBuffComponent::DoesInstanceMatchLegacyType(const FLFPBuffInstance& BuffInstance, ELFPBuffType BuffType) const
{
    if (BuffInstance.bHasLegacyDefinition)
    {
        return BuffInstance.LegacyDefinition.BuffType == BuffType;
    }

    if (BuffType == ELFPBuffType::BT_Bleed)
    {
        return IsBleedInstance(BuffInstance);
    }

    return !IsBleedInstance(BuffInstance);
}

bool ULFPBuffComponent::IsBleedInstance(const FLFPBuffInstance& BuffInstance) const
{
    return (BuffInstance.bHasLegacyDefinition && BuffInstance.LegacyDefinition.BuffType == ELFPBuffType::BT_Bleed) ||
        IsBleedBuffId(BuffInstance.BuffId);
}

int32 ULFPBuffComponent::GetBleedMagnitudeFromInstance(const FLFPBuffInstance& BuffInstance) const
{
    if (!IsBleedInstance(BuffInstance))
    {
        return 0;
    }

    if (BuffInstance.bHasLegacyDefinition)
    {
        int32 LegacyMagnitude = 0;
        for (const FLFPBuffEffectSpec& EffectSpec : BuffInstance.LegacyDefinition.Effects)
        {
            if (EffectSpec.EffectType == ELFPBuffEffectType::BET_PeriodicDamage &&
                EffectSpec.TriggerType == ELFPBuffTriggerType::BTT_OnTurnStart)
            {
                LegacyMagnitude += FMath::Max(EffectSpec.Magnitude, 0) * FMath::Max(BuffInstance.StackCount, 1);
            }
        }
        return LegacyMagnitude;
    }

    int32 Magnitude = 0;
    if (BuffInstance.Definition)
    {
        for (const ULFPBuffEffect* Effect : BuffInstance.Definition->Effects)
        {
            const ULFPBuffEffect_PeriodicDamage* DamageEffect = Cast<ULFPBuffEffect_PeriodicDamage>(Effect);
            if (!DamageEffect || DamageEffect->TriggerEvent != ELFPBuffTriggerEvent::OnTurnStart)
            {
                continue;
            }

            const int32 StackScale = DamageEffect->bScaleByStack ? FMath::Max(BuffInstance.StackCount, 1) : 1;
            Magnitude += FMath::Max(DamageEffect->Damage, 0) * StackScale;
        }
    }

    return Magnitude;
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
    else
    {
        Entry.bVisibleInUI = BuffInstance.LegacyDefinition.bVisibleInUI;
    }

    if (Entry.DisplayName.IsEmpty())
    {
        if (BuffInstance.BuffId.IsValid())
        {
            Entry.DisplayName = FText::FromString(BuffInstance.BuffId.ToString());
        }
        else if (!BuffInstance.LegacyDefinition.SourceSkillName.IsNone())
        {
            Entry.DisplayName = FText::FromName(BuffInstance.LegacyDefinition.SourceSkillName);
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

void ULFPBuffComponent::SyncLegacyBuffsFromInstances()
{
    Buffs.Empty();
    Buffs.Reserve(BuffInstances.Num());

    for (const FLFPBuffInstance& BuffInstance : BuffInstances)
    {
        Buffs.Add(MakeLegacyStateFromInstance(BuffInstance));
    }
}

void ULFPBuffComponent::BroadcastBuffListChanged()
{
    OnBuffListChanged.Broadcast();
}
