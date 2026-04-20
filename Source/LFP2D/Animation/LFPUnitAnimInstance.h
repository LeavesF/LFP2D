#pragma once

#include "CoreMinimal.h"
#include "PaperZDAnimInstance.h"
#include "LFP2D/Unit/LFPUnitAnimationTypes.h"
#include "LFPUnitAnimInstance.generated.h"

class ALFPTacticsUnit;

UCLASS(Blueprintable, BlueprintType, Transient)
class LFP2D_API ULFPUnitAnimInstance : public UPaperZDAnimInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Animation")
    ALFPTacticsUnit* GetOwningUnit() const;

    UFUNCTION(BlueprintPure, Category = "Animation")
    ELFPUnitAnimState GetUnitAnimState() const;

    UFUNCTION(BlueprintPure, Category = "Animation")
    FName GetCurrentAnimationKey() const;

    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsActionAnimationLocked() const;

    UFUNCTION(BlueprintPure, Category = "Animation")
    bool HasPendingAction() const;

    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsHitReactionActive() const;
};
