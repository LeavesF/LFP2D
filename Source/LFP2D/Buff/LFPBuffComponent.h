#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Buff/LFPBuffRuntimeTypes.h"
#include "LFP2D/Buff/LFPBuffTypes.h"
#include "LFPBuffComponent.generated.h"

class ALFPTacticsUnit;
class ULFPBuffDefinitionDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLFPBuffListChangedSignature);

UCLASS(Blueprintable, ClassGroup=(Buff), meta=(BlueprintSpawnableComponent))
class LFP2D_API ULFPBuffComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULFPBuffComponent();

    UPROPERTY(BlueprintAssignable, Category = "Buff|Events")
    FOnLFPBuffListChangedSignature OnBuffListChanged;

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ApplyBuff(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit = nullptr, int32 InitialStackCount = 1, int32 DurationTurnsOverride = -1);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ClearPassiveBuffs();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    bool EvaluateBuffs();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void OnTurnStarted();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void OnTurnEnded();

    UFUNCTION(BlueprintCallable, Category = "Buff")
    int32 RemoveBuffById(FGameplayTag BuffId);

    UFUNCTION(BlueprintCallable, Category = "Buff")
    void ClearAllBuffs();

    UFUNCTION(BlueprintPure, Category = "Buff")
    bool HasAnyBuffs() const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    bool HasBuffById(FGameplayTag BuffId) const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    int32 GetBuffStack(FGameplayTag BuffId) const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    int32 GetTotalBuffCount() const;

    UFUNCTION(BlueprintPure, Category = "Buff")
    FLFPBuffStatModifier GetActiveStatModifier() const;

    UFUNCTION(BlueprintPure, Category = "Buff|Display")
    TArray<FLFPBuffDisplayEntry> GetBuffDisplayEntries(bool bOnlyVisible = false, bool bOnlyActive = false) const;

    UFUNCTION(BlueprintPure, Category = "Buff|Display")
    TArray<FLFPBuffDisplayEntry> GetVisibleBuffDisplayEntries() const;

    // UI 汇总显示用：相同 BuffId 的多个实例会合并为一个条目，并累加层数。
    UFUNCTION(BlueprintPure, Category = "Buff|Display")
    TArray<FLFPBuffDisplayEntry> GetAggregatedVisibleBuffDisplayEntries() const;

    const TArray<FLFPBuffInstance>& GetBuffInstances() const { return BuffInstances; }

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Buff", meta = (AllowPrivateAccess = "true"))
    TArray<FLFPBuffInstance> BuffInstances;

private:
    ALFPTacticsUnit* GetOwnerUnit() const;
    void AddBuffInstance(ULFPBuffDefinitionDataAsset* BuffDefinition, ALFPTacticsUnit* SourceUnit, int32 InitialStackCount, int32 DurationTurnsOverride);
    void ExecuteBuffEffects(FLFPBuffInstance& BuffInstance, ELFPBuffTriggerEvent TriggerEvent, ALFPTacticsUnit* OwnerUnit);
    void CleanupExpiredBuffs();
    bool EvaluateBuffCondition(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const;
    FLFPBuffEffectContext MakeEffectContext(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const;
    FLFPBuffConditionContext MakeConditionContext(const FLFPBuffInstance& BuffInstance, ALFPTacticsUnit* OwnerUnit) const;
    bool IsBleedInstance(const FLFPBuffInstance& BuffInstance) const;
    FLFPBuffDisplayEntry MakeDisplayEntryFromInstance(const FLFPBuffInstance& BuffInstance) const;
    void RefreshInstanceDuration(FLFPBuffInstance& BuffInstance, int32 DurationTurnsOverride) const;
    void BroadcastBuffListChanged();
};
