#pragma once

#include "CoreMinimal.h"
#include "LFPBuffTypes.generated.h"

UENUM(BlueprintType)
enum class ELFPBuffType : uint8
{
    BT_Bleed UMETA(DisplayName = "Bleed"),
    BT_StatModifier UMETA(DisplayName = "Stat Modifier")
};

UENUM(BlueprintType)
enum class ELFPBuffConditionType : uint8
{
    BCT_None UMETA(DisplayName = "None"),
    BCT_NoFriendlyWithinRange UMETA(DisplayName = "No Friendly Within Range")
};

USTRUCT(BlueprintType)
struct FLFPBuffStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    int32 AttackDelta = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    int32 PhysicalBlockDelta = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    int32 SpeedDelta = 0;

    bool IsZero() const
    {
        return AttackDelta == 0 && PhysicalBlockDelta == 0 && SpeedDelta == 0;
    }

    void Append(const FLFPBuffStatModifier& Other)
    {
        AttackDelta += Other.AttackDelta;
        PhysicalBlockDelta += Other.PhysicalBlockDelta;
        SpeedDelta += Other.SpeedDelta;
    }
};

UENUM(BlueprintType)
enum class ELFPBuffLifetimeType : uint8
{
    BLT_TimedTurns UMETA(DisplayName = "Timed Turns"),
    BLT_Infinite UMETA(DisplayName = "Infinite"),
    BLT_WhileConditionTrue UMETA(DisplayName = "While Condition True")
};

UENUM(BlueprintType)
enum class ELFPBuffTriggerType : uint8
{
    BTT_OnTurnStart UMETA(DisplayName = "On Turn Start"),
    BTT_PassiveStat UMETA(DisplayName = "Passive Stat")
};

UENUM(BlueprintType)
enum class ELFPBuffEffectType : uint8
{
    BET_PeriodicDamage UMETA(DisplayName = "Periodic Damage"),
    BET_StatModifier UMETA(DisplayName = "Stat Modifier")
};

USTRUCT(BlueprintType)
struct FLFPBuffEffectSpec
{
    GENERATED_BODY()

    // 效果类型决定“做什么”，例如回合伤害或属性修正。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffEffectType EffectType = ELFPBuffEffectType::BET_StatModifier;

    // 触发类型决定“什么时候生效”。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffTriggerType TriggerType = ELFPBuffTriggerType::BTT_PassiveStat;

    // 通用数值槽，当前主要给周期伤害使用。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    int32 Magnitude = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FLFPBuffStatModifier StatModifier;
};

USTRUCT(BlueprintType)
struct FLFPBuffDefinition
{
    GENERATED_BODY()

    // Definition 描述 Buff 的静态配置，不记录运行时剩余回合等状态。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffType BuffType = ELFPBuffType::BT_StatModifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffLifetimeType LifetimeType = ELFPBuffLifetimeType::BLT_Infinite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0"))
    int32 DurationTurns = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffConditionType ConditionType = ELFPBuffConditionType::BCT_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0"))
    int32 ConditionRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FName SourceSkillName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    bool bVisibleInUI = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    TArray<FLFPBuffEffectSpec> Effects;

    bool HasTimedDuration() const
    {
        return LifetimeType == ELFPBuffLifetimeType::BLT_TimedTurns;
    }
};

USTRUCT(BlueprintType)
struct FLFPBuffRuntimeState
{
    GENERATED_BODY()

    // RuntimeState 保存 Buff 的运行时快照，由 BuffComponent 持有。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPBuffDefinition Definition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 RemainingTurns = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 StackCount = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsConditionMet = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsPersistent = false;

    bool IsExpired() const
    {
        return Definition.HasTimedDuration() && RemainingTurns <= 0;
    }

    bool IsActive() const
    {
        return !IsExpired() && bIsConditionMet;
    }
};

USTRUCT(BlueprintType)
struct FLFPPersistentBuffDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffType BuffType = ELFPBuffType::BT_StatModifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffConditionType ConditionType = ELFPBuffConditionType::BCT_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0"))
    int32 ConditionRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FLFPBuffStatModifier StatModifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    FName SourceSkillName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    bool bVisibleInUI = true;

    // 兼容旧接口：把条件型常驻 Buff 转成统一的 BuffDefinition。
    FLFPBuffDefinition ToBuffDefinition() const
    {
        FLFPBuffDefinition BuffDefinition;
        BuffDefinition.BuffType = BuffType;
        BuffDefinition.LifetimeType = ConditionType == ELFPBuffConditionType::BCT_None
            ? ELFPBuffLifetimeType::BLT_Infinite
            : ELFPBuffLifetimeType::BLT_WhileConditionTrue;
        BuffDefinition.ConditionType = ConditionType;
        BuffDefinition.ConditionRange = ConditionRange;
        BuffDefinition.SourceSkillName = SourceSkillName;
        BuffDefinition.bVisibleInUI = bVisibleInUI;

        if (!StatModifier.IsZero())
        {
            FLFPBuffEffectSpec EffectSpec;
            EffectSpec.EffectType = ELFPBuffEffectType::BET_StatModifier;
            EffectSpec.TriggerType = ELFPBuffTriggerType::BTT_PassiveStat;
            EffectSpec.StatModifier = StatModifier;
            BuffDefinition.Effects.Add(EffectSpec);
        }

        return BuffDefinition;
    }
};

USTRUCT(BlueprintType)
struct FLFPPersistentBuffRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPPersistentBuffDefinition Definition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsActive = false;

    // 兼容旧 UI/查询接口：从统一运行时结构投影出旧的 persistent 视图。
    static FLFPPersistentBuffRuntimeState FromRuntimeState(const FLFPBuffRuntimeState& RuntimeState)
    {
        FLFPPersistentBuffRuntimeState PersistentState;
        PersistentState.Definition.BuffType = RuntimeState.Definition.BuffType;
        PersistentState.Definition.ConditionType = RuntimeState.Definition.ConditionType;
        PersistentState.Definition.ConditionRange = RuntimeState.Definition.ConditionRange;
        PersistentState.Definition.SourceSkillName = RuntimeState.Definition.SourceSkillName;
        PersistentState.Definition.bVisibleInUI = RuntimeState.Definition.bVisibleInUI;

        for (const FLFPBuffEffectSpec& EffectSpec : RuntimeState.Definition.Effects)
        {
            if (EffectSpec.EffectType == ELFPBuffEffectType::BET_StatModifier &&
                EffectSpec.TriggerType == ELFPBuffTriggerType::BTT_PassiveStat)
            {
                PersistentState.Definition.StatModifier.Append(EffectSpec.StatModifier);
            }
        }

        PersistentState.bIsActive = RuntimeState.IsActive();
        return PersistentState;
    }
};
