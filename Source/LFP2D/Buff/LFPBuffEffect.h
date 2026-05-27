#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LFP2D/Buff/LFPBuffRuntimeTypes.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPBuffEffect.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffEffect : public UObject
{
    GENERATED_BODY()

public:
    // 效果触发时机。只有当前事件匹配时，BuffDefinition 才会执行该效果。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    ELFPBuffTriggerEvent TriggerEvent = ELFPBuffTriggerEvent::PassiveStat;

    UFUNCTION(BlueprintPure, Category = "Buff|Effect")
    bool MatchesTrigger(ELFPBuffTriggerEvent InTriggerEvent) const { return TriggerEvent == InTriggerEvent; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Buff|Effect")
    void Execute(const FLFPBuffEffectContext& Context) const;
    virtual void Execute_Implementation(const FLFPBuffEffectContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Buff|Effect")
    FLFPBuffStatModifier GetStatModifier(const FLFPBuffEffectContext& Context) const;
    virtual FLFPBuffStatModifier GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffEffect_PeriodicDamage : public ULFPBuffEffect
{
    GENERATED_BODY()

public:
    ULFPBuffEffect_PeriodicDamage();

    // 每次触发造成的基础真实伤害。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect", meta = (ClampMin = "0"))
    int32 Damage = 0;

    // 是否按 Buff 层数放大伤害；开启后最终伤害 = Damage * StackCount。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    bool bScaleByStack = true;

    virtual void Execute_Implementation(const FLFPBuffEffectContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffEffect_ModifyStat : public ULFPBuffEffect
{
    GENERATED_BODY()

public:
    ULFPBuffEffect_ModifyStat();

    // 被动属性修正值。该效果只在属性重建时汇总，不会在注册 Buff 时直接改单位属性。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    FLFPBuffStatModifier StatModifier;

    // 是否按 Buff 层数放大属性修正。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    bool bScaleByStack = true;

    virtual FLFPBuffStatModifier GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const override;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LFP2D_API ULFPBuffEffect_TerrainStatScaling : public ULFPBuffEffect
{
    GENERATED_BODY()

public:
    ULFPBuffEffect_TerrainStatScaling();

    // 要统计的地形类型。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    ELFPTerrainType TerrainType = ELFPTerrainType::TT_Forest;

    // 以单位所在格为中心，统计半径内的格子范围。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect", meta = (ClampMin = "0"))
    int32 CheckRange = 1;

    // 每个地形格、每层 Buff 提供的属性修正。森林之力用例：AttackDelta = 1。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effect")
    FLFPBuffStatModifier StatPerStackPerTile;

    virtual FLFPBuffStatModifier GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const override;
};
