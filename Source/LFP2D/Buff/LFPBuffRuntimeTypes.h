#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LFP2D/Buff/LFPBuffTypes.h"
#include "LFPBuffRuntimeTypes.generated.h"

class ALFPTacticsUnit;
class ULFPBuffDefinitionDataAsset;
class UTexture2D;

UENUM(BlueprintType)
enum class ELFPBuffCategory : uint8
{
    Buff UMETA(DisplayName = "Buff"),
    Debuff UMETA(DisplayName = "Debuff"),
    Neutral UMETA(DisplayName = "Neutral")
};

UENUM(BlueprintType)
enum class ELFPBuffDurationType : uint8
{
    // 有限回合 Buff，会在目标单位回合开始结算后递减 RemainingTurns。
    TimedTurns UMETA(DisplayName = "Timed Turns"),

    // 无限期 Buff，不会因为回合递减而自动移除，通常用于被动或战斗内常驻效果。
    Infinite UMETA(DisplayName = "Infinite"),

    // 条件成立时生效，条件不成立时保留实例但不参与效果汇总。
    WhileConditionTrue UMETA(DisplayName = "While Condition True")
};

UENUM(BlueprintType)
enum class ELFPBuffStackingMode : uint8
{
    // 每次施加都新增一个独立实例。旧版流血默认使用该规则，所以多个流血各自倒计时。
    AddInstance UMETA(DisplayName = "Add Instance"),

    // 如果目标已有同 BuffId，只刷新持续时间，不增加层数；适合“攻击力提升 2 回合，重复施加只续时”。
    RefreshDuration UMETA(DisplayName = "Refresh Duration"),

    // 如果目标已有同 BuffId，增加层数并刷新持续时间；适合真正的可叠层状态，例如中毒/流血层数。
    AddStackAndRefreshDuration UMETA(DisplayName = "Add Stack And Refresh Duration"),

    // 如果目标已有同 BuffId，先移除旧实例再添加新实例；适合唯一被动或强弱版本互斥的 Buff。
    Replace UMETA(DisplayName = "Replace")
};

UENUM(BlueprintType)
enum class ELFPBuffTriggerEvent : uint8
{
    // Buff 被成功添加且条件满足时触发一次。
    OnApply UMETA(DisplayName = "On Apply"),

    // Buff 被移除或过期时触发一次。
    OnRemove UMETA(DisplayName = "On Remove"),

    // 目标单位回合开始时触发，用于流血、灼烧等周期效果。
    OnTurnStart UMETA(DisplayName = "On Turn Start"),

    // 目标单位回合结束时触发，当前预留。
    OnTurnEnd UMETA(DisplayName = "On Turn End"),

    // 属性汇总触发点，不直接执行，而是在 RebuildCurrentStatsFromRuntimeSources 中读取。
    PassiveStat UMETA(DisplayName = "Passive Stat")
};

UENUM(BlueprintType)
enum class ELFPBuffConditionMatchType : uint8
{
    // Conditions 中所有条件都满足时，Buff 才算激活。
    All UMETA(DisplayName = "All"),

    // Conditions 中任意一个条件满足时，Buff 就算激活。
    Any UMETA(DisplayName = "Any")
};

USTRUCT(BlueprintType)
struct FLFPBuffDurationPolicy
{
    GENERATED_BODY()

    // 持续方式：有限回合、无限期，或条件成立期间生效。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffDurationType DurationType = ELFPBuffDurationType::Infinite;

    // 有限回合 Buff 的默认持续回合数；通过 ApplyBuff 传入 override 时会被覆盖。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0"))
    int32 DurationTurns = 0;
};

USTRUCT(BlueprintType)
struct FLFPBuffStackingPolicy
{
    GENERATED_BODY()

    // 同 BuffId 再次施加时如何处理：新增实例、刷新时间、叠层刷新，或替换旧实例。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    ELFPBuffStackingMode StackingMode = ELFPBuffStackingMode::AddInstance;

    // 叠层上限，仅 AddStackAndRefreshDuration 等会增加 StackCount 的规则使用。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "1"))
    int32 MaxStacks = 99;
};

USTRUCT(BlueprintType)
struct FLFPBuffDisplayEntry
{
    GENERATED_BODY()

    // UI 和蓝图逻辑使用的 Buff ID。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    FGameplayTag BuffId;

    // 运行时实例 ID。AddInstance 模式下，同 BuffId 会有多个不同实例。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    FString InstanceId;

    // UI 显示名称；数据资产未配置时回退为 BuffId。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    FText DisplayName;

    // UI 描述文本。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    FText Description;

    // UI 图标。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    TObjectPtr<UTexture2D> Icon = nullptr;

    // Buff / Debuff / Neutral 分类。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    ELFPBuffCategory Category = ELFPBuffCategory::Buff;

    // 当前层数。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    int32 StackCount = 1;

    // 剩余回合数；无限期 Buff 通常为 0。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    int32 RemainingTurns = 0;

    // 是否是 UI 可见 Buff。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    bool bVisibleInUI = true;

    // 当前是否激活。条件不满足时实例仍存在，但该值为 false。
    UPROPERTY(BlueprintReadOnly, Category = "Buff|Display")
    bool bIsActive = true;
};

USTRUCT(BlueprintType)
struct FLFPBuffEffectContext
{
    GENERATED_BODY()

    // Buff 当前作用的目标单位。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> TargetUnit = nullptr;

    // Buff 来源单位；没有来源时通常回退为目标单位。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> SourceUnit = nullptr;

    // 当前 Buff 的数据 ID，用于查询、UI 和跨系统判断。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    FGameplayTag BuffId;

    // 当前实例层数。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 StackCount = 1;

    // 当前剩余回合数；无限期 Buff 通常为 0。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 RemainingTurns = 0;
};

USTRUCT(BlueprintType)
struct FLFPBuffConditionContext
{
    GENERATED_BODY()

    // 条件检查的目标单位。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> TargetUnit = nullptr;

    // Buff 来源单位；用于“来源存活”等后续条件。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> SourceUnit = nullptr;

    // 当前 Buff 的数据 ID。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    FGameplayTag BuffId;

    // 当前实例层数。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 StackCount = 1;

    // 当前剩余回合数。
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 RemainingTurns = 0;
};

USTRUCT(BlueprintType)
struct FLFPBuffInstance
{
    GENERATED_BODY()

    // 静态配置资产；运行时实例不直接存储策划配置细节。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ULFPBuffDefinitionDataAsset> Definition = nullptr;

    // Buff 唯一配置 ID。新系统使用 GameplayTag 替代旧 ELFPBuffType 枚举。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FGameplayTag BuffId;

    // 施加 Buff 的来源单位。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> SourceUnit = nullptr;

    // 持有 Buff 的目标单位。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    TObjectPtr<ALFPTacticsUnit> TargetUnit = nullptr;

    // 实例当前采用的持续方式，来自 Definition，可在运行时复制出来便于判断。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ELFPBuffDurationType DurationType = ELFPBuffDurationType::Infinite;

    // 有限回合 Buff 的剩余回合。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 RemainingTurns = 0;

    // 当前实例层数。AddInstance 会产生多个 StackCount=1 的实例；叠层规则会累加该值。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 StackCount = 1;

    // 条件是否满足；不满足时实例保留，但 IsActive 返回 false。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsConditionMet = true;

    // 兼容旧接口：非有限回合 Buff 视为 persistent，用于 ClearPersistentBuffs。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsPersistent = false;

    // 运行时唯一实例 ID，后续 UI 或日志可以用它区分同 BuffId 的多个实例。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FGuid InstanceId;

    // 当前实例是否由旧 FLFPBuffDefinition 转换而来。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bHasLegacyDefinition = false;

    // 旧系统兼容快照，用于 GetBuffStates / GetPersistentBuffStates 等旧查询。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPBuffDefinition LegacyDefinition;

    bool HasTimedDuration() const
    {
        return DurationType == ELFPBuffDurationType::TimedTurns;
    }

    bool IsExpired() const
    {
        return HasTimedDuration() && RemainingTurns <= 0;
    }

    bool IsActive() const
    {
        return !IsExpired() && bIsConditionMet;
    }
};
