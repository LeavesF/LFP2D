#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "LFP2D/Buff/LFPBuffRuntimeTypes.h"
#include "LFPBuffCondition.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffCondition : public UObject
{
    GENERATED_BODY()

public:
    // 返回 true 表示当前 Buff 实例满足该条件。
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Buff|Condition")
    bool IsMet(const FLFPBuffConditionContext& Context) const;
    virtual bool IsMet_Implementation(const FLFPBuffConditionContext& Context) const;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffCondition_NoFriendlyWithinRange : public ULFPBuffCondition
{
    GENERATED_BODY()

public:
    // 检查范围，按六边格距离计算。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition", meta = (ClampMin = "0"))
    int32 Range = 0;

    // 是否排除目标单位自身。孤狼类条件通常应开启。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition")
    bool bExcludeSelf = true;

    virtual bool IsMet_Implementation(const FLFPBuffConditionContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffCondition_HealthPercentBelow : public ULFPBuffCondition
{
    GENERATED_BODY()

public:
    // 血量百分比阈值，0.5 表示低于 50% 时满足。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition", meta = (ClampMin = "0", ClampMax = "1"))
    float Threshold = 0.5f;

    virtual bool IsMet_Implementation(const FLFPBuffConditionContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffCondition_HasBuff : public ULFPBuffCondition
{
    GENERATED_BODY()

public:
    // 目标单位必须拥有的 BuffId。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition")
    FGameplayTag RequiredBuffId;

    // 开启后表示“没有该 BuffId 时满足”。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition")
    bool bInvert = false;

    virtual bool IsMet_Implementation(const FLFPBuffConditionContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffCondition_HasGameplayTag : public ULFPBuffCondition
{
    GENERATED_BODY()

public:
    // 目标单位必须拥有的单位标签。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition")
    FGameplayTag RequiredTag;

    // 开启后表示“没有该标签时满足”。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Condition")
    bool bInvert = false;

    virtual bool IsMet_Implementation(const FLFPBuffConditionContext& Context) const override;
};
