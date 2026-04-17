#pragma once

#include "CoreMinimal.h"
#include "LFPBuffTypes.generated.h"

UENUM(BlueprintType)
enum class ELFPBuffType : uint8
{
    BT_Bleed UMETA(DisplayName = "Bleed")
};

USTRUCT(BlueprintType)
struct FLFPActiveBuff
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ELFPBuffType BuffType = ELFPBuffType::BT_Bleed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 RemainingTurnTriggers = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 DamagePerTrigger = 0;
};
