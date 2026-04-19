#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Buff/LFPBuffTypes.h"
#include "LFPBuffComponent.generated.h"

class ALFPTacticsUnit;

UCLASS(Blueprintable, ClassGroup=(Buff), meta=(BlueprintSpawnableComponent))
class LFP2D_API ULFPBuffComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULFPBuffComponent();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ApplyBleed(int32 BleedStacks, int32 DurationTurns);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void OnTurnStarted();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ClearAllBuffs();

    UFUNCTION(BlueprintPure, Category = "Buff")
    bool HasAnyBuffs() const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    bool HasBuff(ELFPBuffType BuffType) const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    int32 GetBuffCount(ELFPBuffType BuffType) const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    int32 GetBleedStacks() const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    int32 GetTotalBuffCount() const;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Buff", meta = (AllowPrivateAccess = "true"))
    TArray<FLFPActiveBuff> ActiveBuffs;

private:
    ALFPTacticsUnit* GetOwnerUnit() const;
    void CleanupExpiredBuffs();
};
