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
    void RegisterBuff(const FLFPBuffDefinition& BuffDefinition);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void RegisterPersistentBuff(const FLFPPersistentBuffDefinition& BuffDefinition);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ClearPersistentBuffs();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    bool EvaluateBuffs();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    bool EvaluatePersistentBuffs();

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

    UFUNCTION(BlueprintPure, Category = "Buff")
    FLFPBuffStatModifier GetActiveStatModifier() const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    FLFPBuffStatModifier GetActivePersistentStatModifier() const;

    const TArray<FLFPBuffRuntimeState>& GetBuffStates() const { return Buffs; }
    TArray<FLFPPersistentBuffRuntimeState> GetPersistentBuffStates() const;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Buff", meta = (AllowPrivateAccess = "true"))
    TArray<FLFPBuffRuntimeState> Buffs;

private:
    ALFPTacticsUnit* GetOwnerUnit() const;
    void AddBuff(const FLFPBuffDefinition& BuffDefinition, bool bIsPersistent);
    void ExecuteBuffEffects(FLFPBuffRuntimeState& BuffState, ELFPBuffTriggerType TriggerType, ALFPTacticsUnit* OwnerUnit);
    void CleanupExpiredBuffs();
    bool EvaluateBuffCondition(const FLFPBuffDefinition& BuffDefinition, const ALFPTacticsUnit* OwnerUnit) const;
};
