#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LFP2D/Unit/LFPUnitTypes.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillEffect.generated.h"

class ALFPHexTile;
class ALFPTacticsUnit;
class ULFPBuffDefinitionDataAsset;
class ULFPSkillBase;

UENUM(BlueprintType)
enum class ELFPSkillEffectTarget : uint8
{
    // 技能当前目标格上的单位。
    TargetUnit UMETA(DisplayName = "Target Unit"),

    // 技能释放者自身。
    OwnerUnit UMETA(DisplayName = "Owner Unit")
};

UENUM(BlueprintType)
enum class ELFPSkillDamageTypeSource : uint8
{
    // 使用释放者当前攻击类型。
    OwnerAttackType UMETA(DisplayName = "Owner Attack Type"),

    // 使用效果里显式配置的伤害类型。
    Override UMETA(DisplayName = "Override")
};

USTRUCT(BlueprintType)
struct FLFPSkillEffectContext
{
    GENERATED_BODY()

    // 当前执行的技能实例。
    UPROPERTY(BlueprintReadOnly, Category = "Skill|Effect")
    TObjectPtr<ULFPSkillBase> Skill = nullptr;

    // 技能释放者。
    UPROPERTY(BlueprintReadOnly, Category = "Skill|Effect")
    TObjectPtr<ALFPTacticsUnit> OwnerUnit = nullptr;

    // 技能目标格。
    UPROPERTY(BlueprintReadOnly, Category = "Skill|Effect")
    TObjectPtr<ALFPHexTile> TargetTile = nullptr;

    // 目标格上的单位；目标为空格时为 nullptr。
    UPROPERTY(BlueprintReadOnly, Category = "Skill|Effect")
    TObjectPtr<ALFPTacticsUnit> TargetUnit = nullptr;
};

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPSkillEffect : public UObject
{
    GENERATED_BODY()

public:
    // 效果作用对象。大多数攻击技能使用 TargetUnit，自身增益使用 OwnerUnit。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    ELFPSkillEffectTarget Target = ELFPSkillEffectTarget::TargetUnit;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Effect")
    bool CanApply(const FLFPSkillEffectContext& Context) const;
    virtual bool CanApply_Implementation(const FLFPSkillEffectContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Effect")
    void Apply(const FLFPSkillEffectContext& Context) const;
    virtual void Apply_Implementation(const FLFPSkillEffectContext& Context) const;

protected:
    ALFPTacticsUnit* ResolveTargetUnit(const FLFPSkillEffectContext& Context) const;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPSkillEffect_DealOwnerSkillDamage : public ULFPSkillEffect
{
    GENERATED_BODY()

public:
    ULFPSkillEffect_DealOwnerSkillDamage();

    virtual bool CanApply_Implementation(const FLFPSkillEffectContext& Context) const override;
    virtual void Apply_Implementation(const FLFPSkillEffectContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPSkillEffect_ApplyBuff : public ULFPSkillEffect
{
    GENERATED_BODY()

public:
    ULFPSkillEffect_ApplyBuff();

    // 要施加的 Buff 数据资产。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect", meta = (AllowedClasses = "/Script/LFP2D.LFPBuffDefinitionDataAsset"))
    TObjectPtr<ULFPBuffDefinitionDataAsset> BuffDefinition = nullptr;

    // 初始层数。叠层规则由 BuffDefinition 的 StackingPolicy 决定。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect", meta = (ClampMin = "1"))
    int32 StackCount = 1;

    // 持续回合覆盖值。小于 0 时使用 BuffDefinition 自己配置的 DurationTurns。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    int32 DurationTurnsOverride = -1;

    virtual bool CanApply_Implementation(const FLFPSkillEffectContext& Context) const override;
    virtual void Apply_Implementation(const FLFPSkillEffectContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPSkillEffect_DamageByTargetBuffStack : public ULFPSkillEffect
{
    GENERATED_BODY()

public:
    ULFPSkillEffect_DamageByTargetBuffStack();

    // 要读取层数的 BuffId，例如 Buff.Status.Bleed；未配置时默认读取 Buff.Status.Bleed。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    FGameplayTag RequiredBuffId;

    // 每层 Buff 造成的基础伤害。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect", meta = (ClampMin = "0"))
    int32 DamagePerStack = 1;

    // 伤害类型来源。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect")
    ELFPSkillDamageTypeSource DamageTypeSource = ELFPSkillDamageTypeSource::OwnerAttackType;

    // DamageTypeSource 为 Override 时使用。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Effect", meta = (EditCondition = "DamageTypeSource == ELFPSkillDamageTypeSource::Override"))
    ELFPAttackType OverrideDamageType = ELFPAttackType::AT_Physical;

    virtual bool CanApply_Implementation(const FLFPSkillEffectContext& Context) const override;
    virtual void Apply_Implementation(const FLFPSkillEffectContext& Context) const override;

private:
    int32 GetStackCount(const FLFPSkillEffectContext& Context) const;
    ELFPAttackType ResolveDamageType(const FLFPSkillEffectContext& Context) const;
};
