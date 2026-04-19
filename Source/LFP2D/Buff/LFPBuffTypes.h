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
};

USTRUCT(BlueprintType)
struct FLFPPersistentBuffRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FLFPPersistentBuffDefinition Definition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool bIsActive = false;
};
