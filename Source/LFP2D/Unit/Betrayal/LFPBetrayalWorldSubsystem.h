#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPBetrayalWorldSubsystem.generated.h"

class ALFPTacticsUnit;
class ULFPBetrayalComponent;
class ULFPBetrayalCondition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLFPBetrayalResolvedSignature, ALFPTacticsUnit*, Unit, EUnitAffiliation, OldAffiliation, ULFPBetrayalCondition*, TriggeringCondition);

UCLASS()
class LFP2D_API ULFPBetrayalWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	void RegisterComponent(ULFPBetrayalComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	void UnregisterComponent(ULFPBetrayalComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	bool EvaluateUnit(ALFPTacticsUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Betrayal")
	int32 EvaluateAllUnits();

	UPROPERTY(BlueprintAssignable, Category = "Betrayal")
	FLFPBetrayalResolvedSignature OnBetrayalResolvedInSubsystem;

private:
	UFUNCTION()
	void HandleComponentBetrayal(ALFPTacticsUnit* Unit, EUnitAffiliation OldAffiliation, ULFPBetrayalCondition* TriggeringCondition);

	UPROPERTY()
	TArray<TObjectPtr<ULFPBetrayalComponent>> RegisteredComponents;
};
