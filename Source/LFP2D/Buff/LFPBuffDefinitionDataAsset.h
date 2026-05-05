#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LFP2D/Buff/LFPBuffCondition.h"
#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/Buff/LFPBuffRuntimeTypes.h"
#include "LFPBuffDefinitionDataAsset.generated.h"

UCLASS(Blueprintable, BlueprintType)
class LFP2D_API ULFPBuffDefinitionDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    // Buff 的配置 ID。推荐使用 GameplayTag，例如 Buff.Status.Bleed、Buff.Passive.LoneWolf。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    FGameplayTag BuffId;

    // UI 显示名称。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    FText DisplayName;

    // UI 或策划备注使用的描述文本。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    FText Description;

    // UI 图标；当前只保存数据，具体显示由后续 UI 接入。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    TObjectPtr<UTexture2D> Icon = nullptr;

    // Buff / Debuff / Neutral 分类，主要用于 UI 展示和筛选。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    ELFPBuffCategory Category = ELFPBuffCategory::Buff;

    // 持续时间策略：有限回合、无限期，或条件成立期间生效。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPBuffDurationPolicy DurationPolicy;

    // 叠层策略：同 BuffId 重复施加时如何合并或替换。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPBuffStackingPolicy StackingPolicy;

    // 条件列表的匹配方式：All 表示全部满足，Any 表示任意满足。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    ELFPBuffConditionMatchType ConditionMatchType = ELFPBuffConditionMatchType::All;

    // 是否显示在 UI 中；隐藏 Buff 仍会参与战斗计算。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bVisibleInUI = true;

    // 激活条件列表。为空时视为无条件激活。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Buff|Conditions")
    TArray<TObjectPtr<ULFPBuffCondition>> Conditions;

    // 效果列表。一个 Buff 可以同时拥有周期伤害、属性修正等多个效果。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Buff|Effects")
    TArray<TObjectPtr<ULFPBuffEffect>> Effects;

    // 根据 ConditionMatchType 汇总条件结果。
    bool AreConditionsMet(const FLFPBuffConditionContext& Context) const;

    // 执行指定触发时机的效果。
    void ExecuteEffects(ELFPBuffTriggerEvent TriggerEvent, const FLFPBuffEffectContext& Context) const;

    // 汇总 PassiveStat 类型的属性修正；不会直接修改单位属性。
    FLFPBuffStatModifier GetStatModifier(const FLFPBuffEffectContext& Context) const;
};
