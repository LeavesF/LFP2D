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
