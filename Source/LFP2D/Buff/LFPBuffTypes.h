#pragma once

#include "CoreMinimal.h"
#include "LFPBuffTypes.generated.h"

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

    // 最终伤害倍率，默认 1 表示不改变伤害；多个 Buff 同时存在时按乘算合并。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0.0"))
    float OutgoingDamageMultiplier = 1.0f;

    bool IsZero() const
    {
        return AttackDelta == 0 &&
            PhysicalBlockDelta == 0 &&
            SpeedDelta == 0 &&
            FMath::IsNearlyEqual(OutgoingDamageMultiplier, 1.0f);
    }

    void Append(const FLFPBuffStatModifier& Other)
    {
        AttackDelta += Other.AttackDelta;
        PhysicalBlockDelta += Other.PhysicalBlockDelta;
        SpeedDelta += Other.SpeedDelta;
        OutgoingDamageMultiplier *= Other.OutgoingDamageMultiplier;
    }
};
