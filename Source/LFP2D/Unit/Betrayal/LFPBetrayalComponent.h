#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPBetrayalComponent.generated.h"

class ALFPTacticsUnit;
class ULFPBetrayalCondition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLFPBetrayalComponentResolvedSignature, ALFPTacticsUnit*, Unit, EUnitAffiliation, OldAffiliation, ULFPBetrayalCondition*, TriggeringCondition);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LFP2D_API ULFPBetrayalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULFPBetrayalComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ConfigureFromTemplates(const TArray<TObjectPtr<ULFPBetrayalCondition>>& ConditionTemplates);

	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	bool EvaluateBetrayalConditions();

	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	bool TryBetrayToPlayer(ULFPBetrayalCondition* TriggeringCondition = nullptr);

	UFUNCTION(BlueprintPure, Category = "Betrayal")
	ALFPTacticsUnit* GetOwnerUnit() const;

	UPROPERTY(BlueprintAssignable, Category = "Betrayal")
	FLFPBetrayalComponentResolvedSignature OnBetrayalResolved;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Betrayal")
	TArray<TObjectPtr<ULFPBetrayalCondition>> BetrayalConditions;

private:
	void RegisterBetrayalConditions();
	void UnregisterBetrayalConditions();

	bool bBetrayalConditionsRegistered = false;
};
